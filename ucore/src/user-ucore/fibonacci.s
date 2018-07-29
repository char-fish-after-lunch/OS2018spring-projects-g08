          .text                         
          .globl main                   

          .data                         
          .align 2                      
_Main:                                  # virtual table
          .word 0                       # parent
          .word _STRING0                # class name

          .data                         
          .align 2                      
_Fibonacci:                             # virtual table
          .word 0                       # parent
          .word _STRING1                # class name
          .word _Fibonacci.get          



          .section .text                
_PrintInt:                              
          lw    a0, 4(sp)               
          tail  _catlib__PrintInt       
          jr    ra                      
_PrintString:                           
          lw    a0, 4(sp)               
          tail  _catlib__PrintString    
          jr    ra                      
_PrintBool:                             
          lw    a0, 4(sp)               
          tail  _catlib__PrintBool      
          jr    ra                      
_Alloc:                                 
          lw    a0, 4(sp)               
          tail  _catlib__Alloc          
          jr    ra                      
_ReadInteger:                           
          tail  _catlib__ReadInteger    
          jr    ra                      
_ReadLine:                              
          tail  _catlib__ReadLine       
          jr    ra                      
_MUL:                                   
          lw    a0, 4(sp)               
          lw    a1, 8(sp)               
          tail  _catlib__MUL            
          jr    ra                      
_DIV:                                   
          lw    a0, 4(sp)               
          lw    a1, 8(sp)               
          tail  _catlib__DIV            
          jr    ra                      
_MOD:                                   
          lw    a0, 4(sp)               
          lw    a1, 8(sp)               
          tail  _catlib__MOD            
          jr    ra                      
_StringEqual:                           
          lw    a0, 4(sp)               
          lw    a1, 8(sp)               
          tail  _catlib__StringEqual    
          jr    ra                      
_Halt:                                  
          tail  _catlib__Halt           
          jr    ra                      
_Main_New:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -16              
_L18:                                   
          li    s2, 4                   
          sw    s2, 4(sp)               
          call  _Alloc                  
          move  s2, a0                  
          la    s3, _Main               
          sw    s3, 0(s2)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Fibonacci_New:                         # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -16              
_L19:                                   
          li    s2, 4                   
          sw    s2, 4(sp)               
          call  _Alloc                  
          move  s2, a0                  
          la    s3, _Fibonacci          
          sw    s3, 0(s2)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

main:                                   # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -28              
_L20:                                   
          call  _Fibonacci_New          
          move  s2, a0                  
          mv    s3, s2                  
          li    s2, 0                   
          mv    s4, s2                  
          sw    s4, -8(s0)              
          sw    s3, -12(s0)             
_L21:                                   
          li    s2, 10                  
          lw    s3, -8(s0)              
          slt   s4, s3, s2              
          sw    s3, -8(s0)              
          beqz  s4, _L23                
_L22:                                   
          lw    s2, -12(s0)             
          sw    s2, 4(sp)               
          lw    s3, -8(s0)              
          sw    s3, 8(sp)               
          lw    s4, 0(s2)               
          lw    s5, 8(s4)               
          sw    s3, -8(s0)              
          sw    s2, -12(s0)             
          jalr  s5                      
          move  s4, a0                  
          lw    s3, -8(s0)              
          lw    s2, -12(s0)             
          sw    s4, 4(sp)               
          sw    s3, -8(s0)              
          sw    s2, -12(s0)             
          call  _PrintInt               
          lw    s3, -8(s0)              
          lw    s2, -12(s0)             
          la    s4, _STRING2            
          sw    s4, 4(sp)               
          sw    s3, -8(s0)              
          sw    s2, -12(s0)             
          call  _PrintString            
          lw    s3, -8(s0)              
          lw    s2, -12(s0)             
          li    s4, 1                   
          add   s5, s3, s4              
          mv    s3, s5                  
          sw    s3, -8(s0)              
          sw    s2, -12(s0)             
          j     _L21                    
_L23:                                   
          call  _ReadInteger            
          move  s2, a0                  
          mv    s3, s2                  
          lw    s2, -12(s0)             
          sw    s2, 4(sp)               
          sw    s3, 8(sp)               
          lw    s3, 0(s2)               
          lw    s2, 8(s3)               
          jalr  s2                      
          move  s3, a0                  
          sw    s3, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Fibonacci.get:                         # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -24              
_L24:                                   
          li    s2, 2                   
          lw    s3, 8(s0)               
          slt   s4, s3, s2              
          sw    s3, 8(s0)               
          beqz  s4, _L26                
_L25:                                   
          li    s2, 1                   
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      
_L26:                                   
          li    s2, 1                   
          lw    s3, 8(s0)               
          sub   s4, s3, s2              
          lw    s2, 4(s0)               
          sw    s2, 4(sp)               
          sw    s4, 8(sp)               
          lw    s4, 0(s2)               
          lw    s5, 8(s4)               
          sw    s2, 4(s0)               
          sw    s3, 8(s0)               
          jalr  s5                      
          move  s4, a0                  
          lw    s2, 4(s0)               
          lw    s3, 8(s0)               
          li    s5, 2                   
          sub   s6, s3, s5              
          sw    s2, 4(sp)               
          sw    s6, 8(sp)               
          lw    s5, 0(s2)               
          lw    s6, 8(s5)               
          sw    s2, 4(s0)               
          sw    s4, -8(s0)              
          sw    s3, 8(s0)               
          jalr  s6                      
          move  s5, a0                  
          lw    s2, 4(s0)               
          lw    s4, -8(s0)              
          lw    s3, 8(s0)               
          add   s6, s4, s5              
          sw    s2, 4(s0)               
          sw    s3, 8(s0)               
          mv    a0, s6                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

          .section .rodata              
_STRING2:
          .string "\n"                  
_STRING1:
          .string "Fibonacci"           
_STRING0:
          .string "Main"                
