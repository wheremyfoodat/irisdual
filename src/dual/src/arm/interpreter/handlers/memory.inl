
using Bus = Memory::Bus;

auto ReadByte(u32 address) -> u32 {
  return m_memory.ReadByte(address, Bus::Data);
}

auto ReadHalf(u32 address) -> u32 {
  return m_memory.ReadHalf(address, Bus::Data);
}

auto ReadWord(u32 address) -> u32 {
  return m_memory.ReadWord(address, Bus::Data);
}

auto ReadHalfCode(u32 address) -> u32 {
  return m_memory.ReadHalf(address, Bus::Code);
}

auto ReadWordCode(u32 address) -> u32 {
  return m_memory.ReadWord(address, Bus::Code);
}

auto ReadByteSigned(u32 address) -> u32 {
  u32 value = m_memory.ReadByte(address, Bus::Data);

  if(value & 0x80) {
    value |= 0xFFFFFF00;
  }

  return value;
}

auto ReadHalfMaybeRotate(u32 address) -> u32 {
  u32 value = m_memory.ReadHalf(address, Bus::Data);
  
  if((address & 1) && m_model == Model::ARM7) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

auto ReadHalfSigned(u32 address) -> u32 {
  if((address & 1) && m_model == Model::ARM7) {
    return ReadByteSigned(address);
  }

  u32 value = m_memory.ReadHalf(address, Bus::Data);
  if(value & 0x8000) {
    return value | 0xFFFF0000;
  }
  return value;
}

auto ReadWordRotate(u32 address) -> u32 {
  auto value = m_memory.ReadWord(address, Bus::Data);
  auto shift = (address & 3) * 8;

  if(!m_unaligned_data_access_enable) {
    value = (value >> shift) | (value << (32 - shift));
  }
  
  return value;
}

void WriteByte(u32 address, u8  value) {
  m_memory.WriteByte(address, value, Bus::Data);
}

void WriteHalf(u32 address, u16 value) {
  m_memory.WriteHalf(address, value, Bus::Data);
}

void WriteWord(u32 address, u32 value) {
  m_memory.WriteWord(address, value, Bus::Data);
}
