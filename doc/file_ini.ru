**********************************************************************
              Документация на файлы инициализации проекта
----------------------------------------------------------------------
Автор и разработчик : Nickols_K (1995-2000)
**********************************************************************

                              Вступление
                              ~~~~~~~~~~

  Эта полезная библиотека для C - программ написана мною после  долгих
часов работы над библиотекой DOS-овских окошек,  таких как меню,  окон
списков check и radio кнопок, редактора. После того  как  размер  кода
перевалил за 100 kb и библиотека ещё была  далека  от  совершенства  я
понял,  что  для  настройки  большинства  программ  интерактивность  -
излишество, а для многих  промежуточных  и  вспомогательных  утилит  -
губительна.  Отсутствие  возможности   непосредственного   общения   с
пользователем  легко  компенсируется   введением   нужных   ключей   в
инициализационные файлы, к тому же, как правило, все  ситуации  бывают
известны ещё на стадии разработки программ.
  Я принял за  основу  формат  Windows.ini  файлов,  немного  увеличив
глубину секций и добавил в него  скромный  препроцессор.

                               Введение
                               ~~~~~~~~

Библиотека для работы с  файлами  инициализации  проекта  представляет
собой реализацию идеи передачи  параметров  программе  и  их  хранения
через файл инициализации. Файл  инициализации  программы  представляет
собой ASCII-файл, состоящий из записей  секций,  подсекций  и  записей
класса:  переменная=значение  Одноимённые  переменные,  записанные   в
разных секциях подсекциях - считаются разными. Переменная  может  быть
объявлена вне подсекции или секции. Полное имя переменной  состоит  из
секции, подсекции и  собственно  имени  переменной.  Секция  открывает
блок  логически  связанных  между  собой   подсекций   и   переменных.
Подсекция,  соответственно,  открывает  под-блок логически   связанных
переменных. Основная идея создания такой иерархии заключается  в  том,
что один .ini файл может использоваться несколькими программами.

                               Описание
                               ~~~~~~~~
Структура .ini файла:

[ в квадратных скобках ] - Описывается секция. Секция - глобальная тема
                           для группы описателей. Например, описывает
                           программу или группу программ.
                              . . .
                             [ Boot ]
                              . . .
                             [ Root ]
                              . . .
< в угловатых скобках > - Описывается подсекция. Подсекция равносильна
                          секции, но находится на другом уровне тем.
                          Например  описывает  подразделы   какой-либо
                          программы.
                                  . . .
                               < Subtopic1 >
                                  . . .
                               < Subtopic2 >
                                   . . .
name   =       value    - name - логическая переменная, сопоставляемая
                          с какой-либо переменной или флагом программы.
                          value - значение этой переменной. В принципе
                          любая строковая константа.
                          Например:
                                 . . .
                            Radius  =  100.098
                            SmartCompute = yes
                            Output = display
                            OutFile = result.res
                                 . . .
;                      - Символ комментария. Считываемые данные
                         считаются закомментированными от места встречи
                         символа ';' и до конца строки.
                         Например:
                            . . .
                          ; - это комментарий
                          debug = full ; possible - full, off , min
                            . . .
#                      - символ препроцессора и файлового процессора:
    #include " fff "   - расширяет эту строку содержанием файла fff.
    #set var = value   - вводит логическую переменную var и ассоциирует
                         её со значением value. Например:
                            . . .
                          #set filename = my_file
                          #set ext      = dat
                          #set suffix   = 001
                            . . .
    #delete var        - удаляет переменную var из списка переменных.
    #reset             - удаляет все переменные.
Внимание !!!!! Использование любой неустановленной или удаленной  пере-
               менной приведет к ошибке (ошибка будет описана в файле
               file_sys.$$$)
Пример к переменным:

                . . .
        #set files = hello
        #set ext   = out
        #set suffix = 001

        Outfiles = %files%%suffix%.%ext% ; ==> Outfiles = hello001.out

        #delete suffix

        Outfiles = %files%%suffix%.%ext% ; ==> Ошибка: suffix удалён
             . . .

    #case             - включает чувствительность программы к регистру
                        (включено по умолчанию)
    #uppercase        - переводит всю считываемую ниже информацию в
                        верхний регистр
    #lowercase        - переводит всю считываемую ниже информацию в
                        нижний регистр
    #smart            - включает расширение переменных заключенных
                        символами % ... % режим принят по умолчанию
    #nosmart          - выключает расширение переменных. Символ %
                        становится обычным символом

    #if               - Открывает условную секцию.
                        За #if должно следовать условие типа:
                        одна из ранее установленных #set переменных
                        .ini файла, признак сравнения:
                        ==, != как в языке Си.
                        и далее значение переменной.
                        Сложные условия, в данной реализации, не
                        поддерживаются, вместо них используйте
                        вложенные #if конструкции.
                        Метод сравнение строк:  strcmp.

    #else             - выполняет условную секцию, если условие было
                        ложно
    #elif             - аналогично секции #else, но с дополнительным
                        условием (см. #if).
    #endif            - закрывает #if секцию

Пример:

          . . .
      #set a = hello
      #if  a == hello
         . . .
      #else
       #if a <= hello
         . . .
       #endif
      #endif
         . . .

    #error "message"  -"message"  - выводит в файл об ошибках сообщение
                                    пользователя

Полный список директив:

#case
#delete
#elif
#else
#endif
#error
#if
#include
#lowercase
#nosmart
#reset
#set
#smart
#uppercase
