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

define which_irqn
  set $irqn = *(volatile uint32_t *) 0xE000ED04
  set $irqn = ($irqn & 0x1FF) - 16
  print "IRQn: %d\n", $irqn
end

define dump_sai1
  # SAI1 register base address (pg. 68 of STM32F446 datasheet)
  set $SAI1_BASE_ADDR = 0x40015800

  # SA1 register offsets (pg. 961 of STM32F446 reference manual)
  set $SAI1A_STATUS_ADDR = $SAI1_BASE_ADDR + 0x018
  set $SAI1B_STATUS_ADDR = $SAI1_BASE_ADDR + 0x038

  set $SAI1A_STATUS  = *(volatile uint32_t *) $SAI1A_STATUS_ADDR
  printf "SAI1A_STATUS [0x%08x]: 0x%08x ", $SAI1A_STATUS_ADDR, $SAI1A_STATUS
  printf "|LFSDET=%d", ($SAI1A_STATUS & 0x40) / 0x40
  printf "|AFSDET=%d", ($SAI1A_STATUS & 0x20) / 0x20
  printf "|CNRDY=%d", ($SAI1A_STATUS & 0x10) / 0x10
  printf "|FREQ=%d", ($SAI1A_STATUS & 0x08) / 0x08
  printf "|WCKCFG=%d", ($SAI1A_STATUS & 0x04) / 0x04
  printf "|MUTEDET=%d", ($SAI1A_STATUS & 0x02) / 0x02
  printf "|OVRUDR=%d|\n", ($SAI1A_STATUS & 0x01) / 0x01

  set $SAI1B_STATUS  = *(volatile uint32_t *) $SAI1B_STATUS_ADDR
  printf "SAI1B_STATUS [0x%08x]: 0x%08x ", $SAI1B_STATUS_ADDR, $SAI1B_STATUS
  printf "|LFSDET=%d", ($SAI1B_STATUS & 0x40) / 0x40
  printf "|AFSDET=%d", ($SAI1B_STATUS & 0x20) / 0x20
  printf "|CNRDY=%d", ($SAI1B_STATUS & 0x10) / 0x10
  printf "|FREQ=%d", ($SAI1B_STATUS & 0x08) / 0x08
  printf "|WCKCFG=%d", ($SAI1B_STATUS & 0x04) / 0x04
  printf "|MUTEDET=%d", ($SAI1B_STATUS & 0x02) / 0x02
  printf "|OVRUDR=%d|\n", ($SAI1B_STATUS & 0x01) / 0x01
end

load_stm32f4
b Default_Handler
