#include <iostream>
#include <unordered_map>
#include "String.hpp"

using namespace Fig::StringClass::DynamicCapacity;

int main()
{
    std::cout << "=== String Test ===\n";

    // 1. 构造
    String s1("Hello");
    String s2("World");
    String s3(U"你好世界"); // UTF-8中文

    std::cout << "s1: " << s1.toBasicString() << "\n";
    std::cout << "s2: " << s2.toBasicString() << "\n";
    std::cout << "s3: " << s3.toBasicString() << "\n";

    // 2. operator+
    String s4 = s1 + String(", ") + s2;
    std::cout << "s4 (s1 + ', ' + s2): " << s4.toBasicString() << "\n";

    // 3. operator+=
    s1 += String("!!!");
    std::cout << "s1 after += '!!!': " << s1.toBasicString() << "\n";

    // 4. operator==
    std::cout << "s1 == 'Hello!!!'? " << (s1 == String("Hello!!!") ? "true" : "false") << "\n";
    std::cout << "s1 == s2? " << (s1 == s2 ? "true" : "false") << "\n";

    // 5. set / at / operator[]
    s2.set(0, 'w'); // 'World' -> 'world'
    std::cout << "s2 after set(0,'w'): " << s2.toBasicString() << "\n";
    std::cout << "s2.at(1): " << String(s2.at(1)).toBasicString() << "\n";
    std::cout << "s2[2]: " << String(s2[2]).toBasicString() << "\n";

    // 6. reverse
    s1.reverse();
    std::cout << "s1 reversed: " << s1.toBasicString() << "\n";

    // 7. clear
    s3.clear();
    std::cout << "s3 cleared, empty? " << (s3.empty() ? "true" : "false") << "\n";

    // 8. reserve & shrink_to_fit
    s4.reserve(100);
    std::cout << "s4 reserved 100, length: " << s4.length() << "\n";
    s4.shrink_to_fit();
    std::cout << "s4 shrink_to_fit done, length: " << s4.length() << "\n";

    // 9. reverse UTF-8 string
    String utf8Str(U"🌟🚀"); // emoji
    std::cout << "utf8Str: " << utf8Str.toBasicString() << "\n";
    utf8Str.reverse();
    std::cout << "utf8Str reversed: " << utf8Str.toBasicString() << "\n";

    // 10. STL

    std::unordered_map<String, String> map = {
        {String("我去"), String("123")}
    };

    std::cout << map[String("我去")];

    return 0;
}
 