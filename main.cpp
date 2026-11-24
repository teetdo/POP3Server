#include <iostream>
#include "pop3servercli.h" // Thay đổi: Include file CLI của POP3

using namespace std;

int main()
{
    cout << "========================================" << endl;
    cout << "Hello POP3 TCP/IP world!" << endl;

    // 2. Khởi tạo đối tượng giao diện dòng lệnh
    // Khi đối tượng này được tạo, nó sẽ tự động khởi tạo POP3Server bên trong nó.
    POP3ServerCLI cli;

    // 3. Chạy vòng lặp nhận lệnh
    // Hàm này kế thừa từ CmdLineInterface, sẽ giữ chương trình chạy
    // cho đến khi admin gõ lệnh "quit".
    cli.run();
    return 0;
}

