# IRC срвер

На сервере работает бот "MyBot" который даёт актуальную информацию по погоде в городе который вы введеёте.
Для обращения к боту нужно отправить ему личное сообщение:

PRIVMSG MyBot :<Город на латинице>
  
Например:

PRIVMSG MyBot :Moscow

Для работы бота нужно установить библиотеку curl:

sudo apt install curl

Для проверки установки пакета:

curl -V
