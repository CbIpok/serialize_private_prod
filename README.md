## 1. Запуск

**Windows (Visual Studio + MSVC)**  
1. Откройте корневой `CMakeLists.txt` в Visual Studio (2019+).  
2. Постройте конфигурацию Debug/Release — генерируются целевые исполняемые файлы `decoder` и `test_foo`.  

**Linux (Ubuntu + GCC)**  
```bash
cd build
cmake ..
make
# получатся бинарники decoder и тесты
```

---

## 2. Структура проекта
Кратко на верхнем уровне:
```
/
├── serializator/      — библиотека Serializer (хедеры + реализация)
├── tests/             — юнит‑тесты на GoogleTest
├── decoder/           — example-приложение
├── raw.bin, raw.json  — тестовые данные
└── CMakeLists.txt     — корневые настройки сборки
```

---

## 3. Демонстрация удовлетворения требований

```cpp
#include "serialize.hpp"

int main() {
    // 1) Конструктор VectorType принимает разные XType
    VectorType vec{
        IntegerType{42},             // целочисленный
        FloatType{3.14},             // дробный
        StringType{"hello world"}    // строковый
    };

    // 2) Serializator::push шаблонен и принимает VectorType
    Serializator s;
    s.push(vec);

    // 3) Полный цикл сериализация ↔ десериализация сохраняет структуру
    Buffer buff = s.serialize();
    auto restored = Serializator::deserialize(buff);

    // Проверка: один элемент и совпадающий VectorType
    assert(restored.size() == 1);
    assert(restored[0].getValue<VectorType>() == vec);

    return 0;
}
```

*Объяснение:*  
- **VectorType{…}** иллюстрирует шаблонный конструктор, принимающий `IntegerType`, `FloatType`, `StringType`.  
- **s.push(vec)** показывает, что `Serializator::push` шаблонен и принимает `Any`‑совместимые типы.  
- Полный цикл `serialize()` → `deserialize()` возвращает идентичный объект.
*Регламентированный main:*  
- Регламентированный main завершается с выводом "1" в stdout.