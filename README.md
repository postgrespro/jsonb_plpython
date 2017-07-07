# jsonb_plpython

## RU
В 9.5 появилась функциональность CREATE TRANSFORM для передачи постгресовых типов данных в хранимые процедуры на произвольном языке. 
Для этого надо написать функции преобразования данных из внутреннего представления постгреса во внутреннее представление 
интерпретатора языка.

Требуется реализовать преобразования для типов JSON и JSONB и языков PL/Python

## EN
Starting with the PostgreSQL 9.5, there is a support of "CREATE TRANSFORM" command which is used for specifying how to adapt a data type to
a procedural language.

It is needed to implement such transforms for JSONB and JSONB types specifically for language PL/Python
