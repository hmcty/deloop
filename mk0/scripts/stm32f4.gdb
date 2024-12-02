target remote localhost:4242

define load_stm32f4
  monitor reset halt
  load
  set language auto
end

define where_fault
  set $fault = $sp+24
  where $fault
end
  
load_stm32f4

