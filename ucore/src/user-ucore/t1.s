          .text                         
          .globl main                   

          .data                         
          .align 2                      
_Main:                                  # virtual table
          .word 0                       # parent
          .word _STRING0                # class name



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
_L13:                                   
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

main:                                   # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -16              
_L14:                                   
          la    s2, _STRING1            
          sw    s2, 4(sp)               
          call  _PrintString            
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

          .section .rodata              
_STRING1:
          .string "hello world\n"       
_STRING0:
          .string "Main"                
