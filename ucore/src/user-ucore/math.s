          .text                         
          .globl main                   

          .data                         
          .align 2                      
_Math:                                  # virtual table
          .word 0                       # parent
          .word _STRING0                # class name

          .data                         
          .align 2                      
_Main:                                  # virtual table
          .word 0                       # parent
          .word _STRING1                # class name



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
_Math_New:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -16              
_L31:                                   
          li    s2, 4                   
          sw    s2, 4(sp)               
          call  _Alloc                  
          move  s2, a0                  
          la    s3, _Math               
          sw    s3, 0(s2)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Main_New:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -16              
_L32:                                   
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

_Math.abs:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -12              
_L33:                                   
          li    s2, 0                   
          lw    s3, 4(s0)               
          slt   s4, s3, s2              
          xori  s4, s4, 1               
          sw    s3, 4(s0)               
          beqz  s4, _L35                
_L34:                                   
          lw    s2, 4(s0)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      
_L35:                                   
          lw    s2, 4(s0)               
          neg   s3, s2                  
          sw    s2, 4(s0)               
          mv    a0, s3                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Math.pow:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -28              
_L37:                                   
          li    s2, 1                   
          mv    s3, s2                  
          li    s2, 0                   
          mv    s4, s2                  
          sw    s4, -8(s0)              
          sw    s3, -12(s0)             
_L39:                                   
          lw    s2, -8(s0)              
          lw    s3, 8(s0)               
          slt   s4, s2, s3              
          sw    s2, -8(s0)              
          sw    s3, 8(s0)               
          beqz  s4, _L41                
_L40:                                   
          lw    s4, -12(s0)             
          sw    s4, 4(sp)               
          lw    s2, 4(s0)               
          sw    s2, 8(sp)               
          sw    s2, 4(s0)               
          call  _MUL                    
          move  s3, a0                  
          lw    s2, 4(s0)               
          mv    s4, s3                  
          sw    s2, 4(s0)               
          sw    s4, -12(s0)             
_L38:                                   
          li    s2, 1                   
          lw    s3, -8(s0)              
          add   s4, s3, s2              
          mv    s3, s4                  
          sw    s3, -8(s0)              
          j     _L39                    
_L41:                                   
          lw    s2, -12(s0)             
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Math.log:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -24              
_L42:                                   
          li    s2, 1                   
          lw    s3, 4(s0)               
          slt   s4, s3, s2              
          sw    s3, 4(s0)               
          beqz  s4, _L44                
_L43:                                   
          li    s2, 1                   
          neg   s3, s2                  
          mv    a0, s3                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      
_L44:                                   
          li    s2, 0                   
          mv    s3, s2                  
          sw    s3, -8(s0)              
_L45:                                   
          li    s2, 1                   
          lw    s3, 4(s0)               
          slt   s4, s2, s3              
          sw    s3, 4(s0)               
          beqz  s4, _L47                
_L46:                                   
          li    s2, 1                   
          lw    s3, -8(s0)              
          add   s4, s3, s2              
          mv    s3, s4                  
          li    s2, 2                   
          lw    s4, 4(s0)               
          sw    s4, 4(sp)               
          sw    s2, 8(sp)               
          sw    s3, -8(s0)              
          call  _DIV                    
          move  s2, a0                  
          lw    s3, -8(s0)              
          mv    s4, s2                  
          sw    s4, 4(s0)               
          sw    s3, -8(s0)              
          j     _L45                    
_L47:                                   
          lw    s2, -8(s0)              
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Math.max:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -12              
_L48:                                   
          lw    s2, 4(s0)               
          lw    s3, 8(s0)               
          slt   s4, s3, s2              
          sw    s2, 4(s0)               
          sw    s3, 8(s0)               
          beqz  s4, _L50                
_L49:                                   
          lw    s2, 4(s0)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      
_L50:                                   
          lw    s2, 8(s0)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

_Math.min:                              # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -12              
_L52:                                   
          lw    s2, 4(s0)               
          lw    s3, 8(s0)               
          slt   s4, s2, s3              
          sw    s2, 4(s0)               
          sw    s3, 8(s0)               
          beqz  s4, _L54                
_L53:                                   
          lw    s2, 4(s0)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      
_L54:                                   
          lw    s2, 8(s0)               
          mv    a0, s2                  
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

main:                                   # function entry
          sw s0, 0(sp)                  
          sw ra, -4(sp)                 
          move s0, sp                   
          addi sp, sp, -20              
_L56:                                   
          li    s2, 1                   
          neg   s3, s2                  
          sw    s3, 4(sp)               
          call  _Math.abs               
          move  s2, a0                  
          sw    s2, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          li    s2, 2                   
          li    s3, 3                   
          sw    s2, 4(sp)               
          sw    s3, 8(sp)               
          call  _Math.pow               
          move  s2, a0                  
          sw    s2, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          li    s2, 16                  
          sw    s2, 4(sp)               
          call  _Math.log               
          move  s2, a0                  
          sw    s2, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          li    s2, 1                   
          li    s3, 2                   
          sw    s2, 4(sp)               
          sw    s3, 8(sp)               
          call  _Math.max               
          move  s2, a0                  
          sw    s2, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          li    s2, 1                   
          li    s3, 2                   
          sw    s2, 4(sp)               
          sw    s3, 8(sp)               
          call  _Math.min               
          move  s2, a0                  
          sw    s2, 4(sp)               
          call  _PrintInt               
          la    s2, _STRING2            
          sw    s2, 4(sp)               
          call  _PrintString            
          move  sp, s0                  
          lw    ra, -4(s0)              
          lw    s0, 0(s0)               
          jr    ra                      

          .section .rodata              
_STRING2:
          .string "\n"                  
_STRING0:
          .string "Math"                
_STRING1:
          .string "Main"                
