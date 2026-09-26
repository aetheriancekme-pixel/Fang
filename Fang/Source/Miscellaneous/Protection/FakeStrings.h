#pragma once
#include <Windows.h>

// makes  fake strings to make it lil bit difficult :v:

class FakeStringss
{
public:
    static void Generate(size_t count = 256);
    static void Clear();

private:
    static char** pool;
    static size_t poolCount;
};