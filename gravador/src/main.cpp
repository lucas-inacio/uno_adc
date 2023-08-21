#include <atomic>
#include <chrono>
using namespace std::chrono_literals;

#include <deque>
#include <iostream>
#include <mutex>
#include <numbers>
#include <thread>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <CLI/CLI.hpp>

#include "SerialPort.h"
#include "Wav.h"

// Resolve conflito com DrawText em WinUser.h
#ifdef DrawText
#undef DrawText
#endif

std::atomic<bool> captura{true};
std::deque<uint16_t> amostras;
std::mutex amostrasMutex;

void captura_amostras(SerialPort &porta)
{
    std::deque<uint8_t> _dados;
    while(captura)
    {
        uint8_t buf[64] = { 0 };
        size_t bytesLidos = porta.read(buf, 64, 10);

        for(size_t i = 0; i < bytesLidos; ++i)
            _dados.push_back(buf[i]);

        // Remove sequência de sincronia
        if(_dados.size() > 2)
        {
            // Se encontrar um byte 0xff que não seja antecedido por outro 0xff,
            // então deve-se descartar o byte anterior, pois este último pertence a uma
            // amostra perdida
            if(_dados[1] == 0xff && _dados[0] != 0xff)
                _dados.pop_front();

            // Analisa as amostras e procura a sequência 0xff 0xff para remover
            for(size_t i = 0; i < _dados.size() - 1; ++i)
            {
                if(_dados[i + 1] == 0xff && _dados[i] == 0xff)
                {
                    _dados.erase(_dados.begin() + i, _dados.begin() + i + 2);
                    break;
                }
            }

            // Distribui os dados para os consumidores
            size_t fim = _dados.size();
            if(fim % 2)
                --fim;

            std::lock_guard lock{amostrasMutex};
            for(size_t i = 0; i < fim - 1; i += 2)
            {
                uint16_t amostra = ((_dados[i + 1] << 8) & 0xff00) | (_dados[i] & 0x00ff);
                amostras.push_back(amostra);
            }
            _dados.erase(_dados.begin(), _dados.begin() + fim);
        }        
    }
}

void loop(SerialPort &porta, WavFile &arquivo)
{
    std::deque<uint16_t> _dados;

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    auto renderer_plot_1 = ftxui::Renderer([&] {
        auto image = ftxui::canvas([&_dados](ftxui::Canvas &c) {
            c.DrawText(0, 0, "A graph");

            // Copia os dados para um buffer temporário que será gravado no arquivo
            std::lock_guard lock{amostrasMutex};
            _dados.insert(_dados.end(), amostras.begin(), amostras.end());

            int largura = c.width();
            int altura = c.height();
            if(amostras.size() > largura)
            {
                // A tela tem um tamanho máximo. Descarta dados antigos que não cabem nela.
                size_t diferenca = amostras.size() - largura;
                amostras.erase(amostras.begin(), amostras.begin() + diferenca);
            }

            // Corrige escala do gráfico para preenchê-lo completamente.
            size_t pontos = amostras.size();
            std::vector<int> ys(pontos);
            for (int x = 0; x < pontos; x++) {
                float fdx = std::move(amostras.front()) * altura / 65536.0f;
                amostras.pop_front();
                ys[x] = altura - static_cast<int>(fdx);
            }
            float dx = static_cast<float>(largura) / pontos;
            float coorX = dx;
            for (int x = 1; pontos > 0 && x < pontos - 1; x++)
            {
                c.DrawPointLine(
                    static_cast<int>(coorX), ys[x],
                    static_cast<int>(coorX) + 1, ys[x + 1],
                    ftxui::Color::Red1
                );
                coorX += dx;
            }
        });

        return image | ftxui::flex;
    });

    auto renderer = ftxui::Renderer([&] {
        return ftxui::vbox({
            ftxui::text(porta.portName()) | ftxui::border,
            renderer_plot_1->Render() | ftxui::border
        });
    });

    ftxui::Loop loop(&screen, renderer);
    auto tarefa = std::thread(captura_amostras, std::ref(porta));
    while(!loop.HasQuitted())
    {
        loop.RunOnce();
        std::this_thread::sleep_for(10ms);
        screen.PostEvent(ftxui::Event::Custom);

        if(_dados.size() > 0)
        {
            uint8_t *buffer = new uint8_t[_dados.size() * 2];
            for(size_t i = 0; i < _dados.size(); ++i)
            {
                uint16_t amostra = _dados[i];
                amostra -= 0x8000; // Normaliza entre -32768/+32767

                uint8_t byteBaixo = static_cast<uint8_t>(amostra & 0x00ff);
                uint8_t byteAlto = static_cast<uint8_t>((amostra >> 8) & 0x00ff);
                buffer[i * 2] = byteBaixo;
                buffer[i * 2 + 1] = byteAlto;
            }
            arquivo.writeData(buffer, static_cast<uint32_t>(_dados.size() * 2));
            _dados.clear();
            delete[] buffer;
        }
    }
    captura = false; // Encerra thread de captura
    tarefa.join();
}

// Grava uma onda senoidal pura de 440Hz com duração de 5 segundos
void teste()
{
    constexpr int tempo = 5;
    constexpr int freq = 440;
    constexpr int taxaAmostragem = 9615;
    constexpr int totalAmostras = taxaAmostragem * tempo;

    uint16_t amostras[totalAmostras * 2] = { 0 };
    for(int i = 0; i < totalAmostras; ++i)
    {
        double amostra = 32767 * (sin(2.0 * std::numbers::pi * freq * static_cast<double>(i) * tempo / totalAmostras) + 1.0);
        uint16_t teste = static_cast<uint16_t>(amostra - 0x8000);
        amostras[i] = teste;
    }

    WavFile arquivo{"E:/teste44.wav"};
    uint8_t *buffer = new uint8_t[totalAmostras * 2];
    for(int i = 0; i < totalAmostras; ++i)
    {
        uint8_t byteBaixo = amostras[i] & 0x00ff;
        uint8_t byteAlto = (amostras[i] >> 8) & 0x00ff;
        buffer[i * 2] = byteBaixo;
        buffer[i * 2 + 1] = byteAlto;
    }
    arquivo.writeData(buffer, totalAmostras * 2);
    delete [] buffer;
}

int main(int argc, char **argv) {
    std::string nomePorta;
    std::string caminho;
    CLI::App app{"Get audio stream from serial port"};

    app.require_subcommand(1, 1);

    auto record = app.add_subcommand("record", "Record audio data in a .wav file");
    record->add_option("port", nomePorta, "serial port name")->required();
    record->add_option("path", caminho, "file name where to save data")->required();
    record->parse_complete_callback(
        [&nomePorta, &caminho]{
            WavFile arquivo{caminho, true};
            SerialPort porta{nomePorta, 256000};
            loop(porta, arquivo);
        });

    auto list = app.add_subcommand("list", "List available ports");
    list->parse_complete_callback(
        []{
            sp_port **port_list;
            if(sp_list_ports(&port_list) != SP_OK)
                throw std::runtime_error("Error listing ports");

            sp_port *port;
            while((port = *port_list++) != nullptr)
            {
                std::cout << port->name << "\n";
            }
        }
    );
    
    try
    {
        app.parse(argc, argv);
    }
    catch(CLI::ParseError &e)
    {
        return app.exit(e);
    }
    catch(std::runtime_error &e)
    {
        std::cerr << e.what() << "\n";
        return -1;
    }

    return EXIT_SUCCESS;
}