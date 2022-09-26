#include <iostream>
#include <string>
#include <memory>

#include <string.h>

using namespace std;

const char* buffer = "heello";

int main(){
    string str = "foo";

    shared_ptr<char> pchar(new char[str.size() + 1]);
    memcpy(pchar.get(), str.c_str(), str.size() + 1);

    cout << reinterpret_cast<unsigned long>(str.c_str()) << endl;
    cout << reinterpret_cast<unsigned long>(pchar.get()) << endl;
}
