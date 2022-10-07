## Командная оболочка для Linux на языке Си
По сути это самописный упрощенный аналог bash на языке Си. Но в тоже время очень функциональный так как позволяется выполнять довольно сложные команды и их цепочки, такие как:
```
cs5600$ echo "Hello" > file.txt; cat file.txt | rev && echo "Done!"
olleH
Done!
```

А также запускать процессы в фоновом режиме:
```
cs5600$ (sleep 1; echo "Done") & echo "One"; echo "Two"; echo "Three";
One
Two
Three
cs5600$ Done
```

> Кома́ндная оболо́чка Unix (англ. Unix shell, часто просто «шелл» или «sh») — командный интерпретатор, используемый в операционных системах семейства 
> Unix, в котором пользователь может либо давать команды операционной системе по отдельности, либо запускать скрипты, состоящие из списка команд. 

Это одна из лабораторных работ по курсу cs5600. Полное описание задания на английском языке можно прочитать [здесь](https://naizhengtan.github.io/22spring/labtutorials/lab2/).

Acknowledgments
This lab is created by Eddie Kohler, with modifications by the CS5600 staff. Lab instructions are adapted from Mike Walfish’s cs202 lab instructions.

### В этой лабораторной работе предоставляется:
+ общий "скелет" разрабатываемой программы (прототипы основных функций с пояснением, что они должны делать),
+ часть кода для парсинга команд,
+ готовый Makefile для сборки,
+ набор из 70 тестов, позволяющий проверить корректность выполнения работы.

### Задача - завершить парсинг команд и реализовать их выполнение:
- реализовать сам процесс запуска программ, выполняющих команды
- перенаправление ввода-вывода
  + `>` перенаправление стандартного потока вывода: `echo "hello" > file.txt`
  + `<` перенаправление стандартного потока ввода: `sort < file.txt`
  + `2>` перенаправление стандартного потока ошибок: `cd /nodir 2> file.txt`
- поддержку конвееров: `ls | sort`
- последовательное и фоновое выполнение команд: `cmd && echo "Success"`
- реализовать встроенные команды: `cd`, `pwd` и `exit`.

### Компиляция и запуск
```
make
./cs5600sh
```

### Запуск тестов
```
make test
```

### Критерий выполнения
Если запустить тесты в стартовом варианте то вы получите подробный лог в котором будет указано 
```
0 of 70 tests passed
```

В процессе выполнения работы вы можете запускать тесты и контролировать свой прогресс. Работа считается выполненной если программа соответсвует всем перечисленным требованиями и все тесты пройдены

```
70 of 70 tests passed
```

Посмотреть все изменения, которые потребовалось внести для решения задачи можно с помощью [сравнения стартового и конечного коммитов](https://github.com/zazicam/simple-shell/compare/c7f54f313ffb7dfb0d26d713af4aeb08ea34ed0f..6131b372d5090b42cf8a6a7460c1ff7f9d20f0c3)
