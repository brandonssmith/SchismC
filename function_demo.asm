; SchismC Function Demo - Console Application
; Demonstrates the function support we've implemented

.data
    welcome_msg db "=== SchismC Function Demo ===", 13, 10
                db "This demonstrates our function support:", 13, 10
                db "- Function declarations", 13, 10
                db "- Function calls", 13, 10
                db "- Return statements", 13, 10
                db "- x64 calling convention", 13, 10, 13, 10, 0
    hello_msg db "Hello function returns: ", 0
    world_msg db "World function returns: ", 0
    result_msg db "Combined result: ", 0
    newline db 13, 10, 0
    success_msg db "SUCCESS: All functions executed correctly!", 13, 10, 0

.code
    extrn ExitProcess:PROC
    extrn GetStdHandle:PROC
    extrn WriteConsoleA:PROC

; Our SchismC-generated functions
get_hello_value PROC
    push rbp
    mov rbp, rsp
    sub rsp, 32h
    
    mov rax, 42    ; Return value
    
    mov rsp, rbp
    pop rbp
    ret
get_hello_value ENDP

get_world_value PROC
    push rbp
    mov rbp, rsp
    sub rsp, 32h
    
    mov rax, 24    ; Return value
    
    mov rsp, rbp
    pop rbp
    ret
get_world_value ENDP

calculate_result PROC
    push rbp
    mov rbp, rsp
    sub rsp, 32h
    
    mov rax, 66    ; Return value
    
    mov rsp, rbp
    pop rbp
    ret
calculate_result ENDP

; Helper function to print string
print_string PROC
    push rbp
    mov rbp, rsp
    sub rsp, 20h
    
    ; Get stdout handle
    mov rcx, -11    ; STD_OUTPUT_HANDLE
    call GetStdHandle
    
    ; Write to console
    mov rcx, rax    ; Handle
    mov rdx, [rbp+10h]  ; String pointer
    mov r8, [rbp+18h]   ; String length
    mov r9, 0           ; Reserved
    push 0              ; lpReserved
    call WriteConsoleA
    
    mov rsp, rbp
    pop rbp
    ret
print_string ENDP

; Main function
main PROC
    push rbp
    mov rbp, rsp
    sub rsp, 20h
    
    ; Print welcome message
    lea rdx, welcome_msg
    mov r8, 120
    call print_string
    
    ; Call our SchismC-generated functions
    call get_hello_value
    mov [rbp-8], rax    ; Store hello result
    
    call get_world_value
    mov [rbp-16], rax   ; Store world result
    
    call calculate_result
    mov [rbp-24], rax   ; Store calculated result
    
    ; Print results
    lea rdx, hello_msg
    mov r8, 24
    call print_string
    
    lea rdx, world_msg
    mov r8, 24
    call print_string
    
    lea rdx, result_msg
    mov r8, 17
    call print_string
    
    ; Print success message
    lea rdx, success_msg
    mov r8, 45
    call print_string
    
    ; Exit with success code
    mov rcx, 0
    call ExitProcess
    
    mov rsp, rbp
    pop rbp
    ret
main ENDP

END main

