
#include<iostream>

// __func__ c99 standard
// __FUNCTION__ defacto standard
constexpr auto out_func = __func__; // warning: ‘__func__’ is not defined outside of function scope

constexpr auto out_func1 = __FUNCTION__;
struct f_r
{
    const char* class_func = __func__;
};

int main ()
{
    auto f = [](){
        std::cout << "lambda " << __func__ << std::endl;
    };
    f();

    std::cout << "normal " << __func__ << std::endl;

    std::cout << "out func " << out_func << std::endl; // top level

    std::cout << "class_func " << f_r().class_func << std::endl; // top level

    auto f1 = [](){
        std::cout << "lambda " << __FUNCTION__ << std::endl;
    };
    f1();

    std::cout << "out func1 " << out_func1 << std::endl; // top level
    return 0;
}