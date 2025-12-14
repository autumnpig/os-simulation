#include <iostream>
#include "storage.h"

int main()
{
    Storage storage;
    const std::string filename = "test.txt";

    try
    {
        // 写文件
        storage.writeFile(filename, "Hello Storage Module!\nThis is a test file.");
        std::cout << "Write file success." << std::endl;

        // 读文件
        std::string content = storage.readFile(filename);
        std::cout << "Read file content:" << std::endl;
        std::cout << content << std::endl;

        // 删除文件
        storage.deleteFile(filename);
        std::cout << "Delete file success." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
