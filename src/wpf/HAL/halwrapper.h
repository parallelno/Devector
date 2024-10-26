#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include <array>

namespace dev 
{
    public ref class HAL
    {
    public:

        int field1;
        float field2;

        // A constructor that sets initial values
        HAL(int f1, float f2) {
            field1 = f1;
            field2 = f2;
        }

        // A method to demonstrate interop
        void DisplayData() {
            System::Console::WriteLine("Field1: {0}, Field2: {1}", field1, field2);
        }

        static int DisplayData2() 
        {
            return 0;
        }
    };
}

