#include <iostream>
#include <locale>
#include <codecvt>
#include "lab_library.hpp"

int main () {
    #ifdef _WIN32
        system("chcp 65001 > nul");  // UTF-8
    #endif

    ProcessHandle h1, h2, h3, h4;


    std::cout << "запуск фоновых задач" << std::endl;

    std::cout << "запускаем " << get_name_command(ListCommand::PING) << std::endl;
    if (run_command(ListCommand::PING, &h1) != 0) {
        std::cerr << "Ошибка запуска ping!" << std::endl;
        return 1;
    }

    std::cout << "запускаем " << get_name_command(ListCommand::TIMEOUT) << std::endl;
    if (run_command(ListCommand::TIMEOUT, &h2) != 0) {
        std::cerr << "Ошибка запуска timeout/sleep!" << std::endl;
        return 1;
    }
    
    std::cout << "запускаем " << get_name_command(ListCommand::IPCONFIG) << std::endl;
    if (run_command(ListCommand::IPCONFIG, &h3) != 0) {
        std::cerr << "Ошибка запуска ipconfig!" << std::endl;
        return 1;
    }
    
    std::cout << "запускаем " << get_name_command(ListCommand::DIR) << std::endl;
    if (run_command(ListCommand::DIR, &h4) != 0) {
        std::cerr << "Ошибка запуска dir/ls!" << std::endl;
        return 1;
    }
    

    std::cout << "даже при работе фоновой задачи, проверим" << std::endl;
    for(int i = 0; i <= 5; i++){
        std::cout << i << " ";
    }
    std::cout << std::endl;

    
    int code1 = wait_for_command(&h1);
    std::cout << "PING завершилась с кодом" << code1 << std::endl;

    int code2 = wait_for_command(&h2);
    std::cout << "TIMEOUT завершилась с кодом" << code2 << std::endl;

    int code3 = wait_for_command(&h3);
    std::cout << "IPCONFIG завершилась с кодом" << code3 << std::endl;

    int code4 = wait_for_command(&h4);
    std::cout << "DIR завершилась с кодом" << code4 << std::endl;

    return 0;
}