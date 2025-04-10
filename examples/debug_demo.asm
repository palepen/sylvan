section .data
    msg db "Hello World", 10, 0
    msg2 db "Bye World", 10, 0
    value dq 42

section .bss
    result resq 1

section .text
    global _start

hello:
    mov rax, 1              ; syscall: write
    mov rdi, 1              ; stdout
    mov rsi, msg            ; pointer to message
    mov rdx, 23             ; message length
    syscall
    ret

_start:
    mov rax, [value]        ; Load value from memory (can watch this)
    add rax, 8              ; Arithmetic operation (step into here)
    mov [result], rax       ; Store to memory (set watchpoint here)

    ; Print message (simple syscall-based way)

    call hello

    ; Exit
    mov rax, 60             ; syscall: exit
    xor rdi, rdi            ; status 0
    syscall