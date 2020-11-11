; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax
    .loop:
        cmp byte [rdi+rax], 0   ;проверяем равен ли текущий символ нуль-терминатору
        je .end                 ;если равен, то выходим
        inc rax                 ;инкрементируем счетчик
        jmp .loop
    .end:
        ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    xor rax, rax
    call string_length          ;считаем длину строки
    mov rdx, rax                ;записываем длину строки в rdx
    mov rsi, rdi                ;указатель на строку записываем в rsi
    mov rax, 1                  ;номер системного вызова write записываем в rax
    mov rdi, 1                  ;записываем номер файлового дескриптора
    syscall
    ret

; Принимает код символа и выводит его в stdout
print_char:
    xor rax, rax
    push rdi
    mov rsi, rsp
    mov rax, 1
    mov rdi, 1
    mov rdx, 1
    syscall
    pop rdi
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    xor rax, rax
    mov rdi, 10
    call print_char
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
    xor rax, rax
    push rbp
    mov rbp, rsp
    mov rax, rdi
    mov rdi, 10
    sub rsp, 256
    dec rbp
    mov byte[rbp], 0
    .loop:
        dec rbp
        xor rdx, rdx
        div rdi
        add rdx, '0'
        mov byte[rbp], dl
        test rax, rax
        jnz .loop
        mov rdi, rbp
        call print_string
        add rsp, 256
        pop rbp
        ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    xor rax, rax
    test rdi, rdi
    jns print_uint
    push rdi
    mov rdi, '-'
    call print_char
    pop rdi
    neg rdi
    jmp print_uint

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
    xor rax, rax
    .loop:
        mov cl, byte [rsi+rax]
        cmp byte [rdi+rax], cl
        jne .end_failure
        cmp byte [rdi+rax], 0
        je .end_success
        inc rax
        jmp .loop
    .end_success:
        mov rax, 1
        ret
    .end_failure:
        mov rax, 0
        ret


; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    xor rax, rax
    dec rsp
    mov rdi, 0
    mov rsi, rsp
    mov rdx, 1
    syscall
    mov rax, [rsp]
    inc rsp
    ret 

section .data
	buf : times 256 db 0

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор

read_word:
    push rbx
    mov r8, rsi             ;размер буфера
    mov r9, rdi             ;адрес буфера
    xor rbx, rbx
    xor rdi, rdi            ;номер файлового дескриптора
    mov rdx, 1              ;читаем 1 символ
    .skip:
        xor rax, rax        ;номер системного вызова
        mov rsi, buf
        syscall
        cmp al, 0
        je .finally
        cmp byte[buf], 0x20
        je .skip
        cmp byte[buf], 0x9
        je .skip
        cmp byte[buf], 0xA
        je .skip
        inc rbx
    .read:
        xor rax, rax
        lea rsi, [buf + rbx]
        syscall
        cmp byte[rsi], 0x20
        je .finally
        cmp byte[rsi], 0x9
        je .finally
        cmp byte[rsi], 0xA
        je .finally
        cmp byte[rsi], 0
        je .finally
        cmp r8, rbx
        jbe .exit
        inc rbx
        jmp .read
    .finally:
        mov byte[buf + rbx], 0
        mov rdx, rbx
        mov rax, buf
        pop rbx
        ret
    .exit:
        xor rax, rax
        pop rbx
        ret
 

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
    xor rax, rax
    xor rsi, rsi
    xor rcx, rcx
    mov r11, 10
    .loop:
        mov sil, [rdi+rcx]
        cmp sil, '0'
        jl .return
        cmp sil, '9'
        jg .return
        sub sil, '0'
        mul r11
        add rax, rsi
        inc rcx
        jmp .loop
    .return:
        mov rdx, rcx
        ret




; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    xor rax, rax
    cmp byte[rdi], '-'
    jne parse_uint
    inc rdi
    call parse_uint
    neg rax
    inc rdx
    ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rax, rax
    xor rcx, rcx
    push r11
    .loop:
        cmp rcx, rdx
        je .complete
        mov r11, [rdi+rcx]
        mov [rsi+rcx], r11
        cmp r11, 0
        je .exit
        inc rcx
        jmp .loop
    .complete:
        mov rax, 0
    .exit:
        pop r11
        ret
