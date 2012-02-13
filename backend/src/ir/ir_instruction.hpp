/* 
 * Copyright © 2012 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Segovia <benjamin.segovia@intel.com>
 */

#ifndef __GBE_IR_INSTRUCTION_HPP__
#define __GBE_IR_INSTRUCTION_HPP__

#include "sys/platform.hpp"
#include "ir_register.hpp"
#include "ir_value.hpp"
#include "ir_type.hpp"

namespace gbe
{
  /*! All opcodes */
  enum Opcode : uint8_t {
#define DECL_INSN(INSN, FAMILY) OP_##INSN,
#include "ir_instruction.hxx"
#undef DECL_INSN
  };

  /*! Different memory spaces */
  enum MemorySpace : uint8_t {
    MEM_GLOBAL = 0, //!< Global memory (a la OCL)
    MEM_LOCAL,      //!< Local memory (thread group memory)
    MEM_CONSTANT,   //!< Immutable global memory
    MEM_PRIVATE     //!< Per thread private memory
  };

  /*! A label is identified with an unsigned short */
  typedef uint16_t LabelIndex;

  /*! A value is stored in a per-function vector. This is the index to it */
  typedef uint16_t ValueIndex;

  /*! Function class contains the register file and the register tuple. Any
   *  information related to the registers may therefore require a function
   */
  class Function;

  ///////////////////////////////////////////////////////////////////////////
  /// All public instruction classes as manipulated by all public classes
  ///////////////////////////////////////////////////////////////////////////

  /*! Store the instruction description in 8 bytes */
  class ALIGNED(sizeof(uint64_t)) Instruction
  {
  public:
    /*! Get the instruction opcode */
    INLINE Opcode getOpcode(void) const { return opcode; }
    /*! Get the number of sources for this instruction  */
    uint32_t getSrcNum(void) const;
    /*! Get the number of destination for this instruction */
    uint32_t getDstNum(void) const;
    /*! Get the register index of the given source */
    RegisterIndex getSrcIndex(const Function &fn, uint32_t ID = 0u) const;
    /*! Get the register index of the given destination */
    RegisterIndex getDstIndex(const Function &fn, uint32_t ID = 0u) const;
    /*! Get the register of the given source */
    Register getDst(const Function &fn, uint32_t ID = 0u) const;
    /*! Get the register of the given destination */
    Register getSrc(const Function &fn, uint32_t ID = 0u) const;
    /*! Check that the instruction is well formed. Return true if well formed.
     *  j
     *  Otherwise, fill the string with a help message
     */
    bool check(void) const;
    /*! Indicates if the instruction belongs to instruction type T. Typically, T
     *  can be BinaryInstruction, UnaryInstruction, LoadInstruction and so on
     */
    template <typename T> INLINE bool isMemberOf(void) const {
      return T::isClassOf(*this);
    }
  protected:
    Opcode opcode;                             //!< Idendifies the instruction
    uint8_t opaque[sizeof(uint64_t)-sizeof(uint8_t)];//!< Remainder of it
  };

  // Check that the instruction is properly formed by the compiler
  STATIC_ASSERT(sizeof(Instruction) == sizeof(uint64_t));

  /*! Unary instructions are typed. dst and sources share the same type */
  class UnaryInstruction : public Instruction {
  public:
    /*! Get the type manipulated by the instruction */
    Type getType(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Binary instructions are typed. dst and sources share the same type */
  class BinaryInstruction : public Instruction {
  public:
    /*! Get the type manipulated by the instruction */
    Type getType(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Ternary instructions is mostly for MADs */
  class TernaryInstruction : public Instruction {
  public:
    /*! Get the type manipulated by the instruction */
    Type getType(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Conversion instruction converts from one type to another */
  class ConvertInstruction : public Instruction {
  public:
    /*! Get the type of the source */
    Type getSrcType(void) const;
    /*! Get the type of the destination */
    Type getDstType(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Store instruction. First source is the address. Next sources are the
   *  values to store contiguously at the given address
   */
  class StoreInstruction : public Instruction {
  public:
    /*! Return the types of the values to store */
    Type getValueType(void) const;
    /*! Give the number of values the instruction is storing (srcNum-1) */
    uint32_t getValueNum(void) const;
    /*! Address space that is manipulated here */
    MemorySpace getAddressSpace(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Load instruction. The source is simply the address where to get the data.
   *  The multiple destinations are the contiguous values loaded at the given
   *  address
   */
  class LoadInstruction : public Instruction {
  public:
    /*! Type of the loaded values (ie type of all the destinations) */
    Type getValueType(void) const;
    /*! Number of values loaded (ie number of destinations) */
    uint32_t getValueNum(void) const;
    /*! Address space that is manipulated here */
    MemorySpace getAddressSpace(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Load immediate instruction loads an typed immediate value into the given
   * register. Since double and uint64_t values will not fit into an instruction,
   * the immediate themselves are stored in the function core. Contrary to
   * regular load instructions, there is only one destination possible
   */
  class LoadImmInstruction : public Instruction {
  public:
    /*! Return the value stored in the instruction */
    Value getValue(const Function &fn) const;
    /*! Return the type of the stored value */
    Type getType(void) const;
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Branch instruction is the unified way to branch (with or without
   *  predicate)
   */
  class BranchInstruction : public Instruction {
  public:
    /*! Indicate if the branch is predicated */
    bool isPredicated(void) const;
    /*! Return the predicate register (if predicated) */
    Register getPredicate(const Function &fn) const {
      assert(this->isPredicated() == true);
      return this->getSrc(fn, 0);
    }
    /*! Return the predicate register index (if predicated) */
    RegisterIndex getPredicateIndex(const Function &fn) const {
      assert(this->isPredicated() == true);
      return this->getSrcIndex(fn, 0);
    }
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Label instruction are actual no-op but are referenced by branches as their
   *  targets
   */
  class LabelInstruction : public Instruction {
  public:
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Texture instruction are used for any texture mapping requests */
  class TextureInstruction : public Instruction {
  public:
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Fence instructions are used to order loads and stores for a given memory
   *  space
   */
  class FenceInstruction : public Instruction {
  public:
    /*! Return true if the given instruction is an instance of this class */
    static bool isClassOf(const Instruction &insn);
  };

  /*! Specialize the instruction. Also performs typechecking first based on the
   *  opcode. Crashes if it fails
   */
  template <typename T>
  INLINE T *cast(Instruction *insn) {
    assert(insn->isMemberOf<T>() == true);
    return reinterpret_cast<T*>(insn);
  }
  template <typename T>
  INLINE const T *cast(const Instruction *insn) {
    assert(insn->isMemberOf<T>() == true);
    return reinterpret_cast<const T*>(insn);
  }
  template <typename T>
  INLINE T &cast(Instruction &insn) {
    assert(insn.isMemberOf<T>() == true);
    return reinterpret_cast<T&>(insn);
  }
  template <typename T>
  INLINE const T &cast(const Instruction &insn) {
    assert(insn.isMemberOf<T>() == true);
    return reinterpret_cast<const T&>(insn);
  }

  ///////////////////////////////////////////////////////////////////////////
  /// All emission functions
  ///////////////////////////////////////////////////////////////////////////

  /*! mov.type dst src */
  Instruction mov(Type type, RegisterIndex dst, RegisterIndex src);
  /*! cos.type dst src */
  Instruction cos(Type type, RegisterIndex dst, RegisterIndex src);
  /*! sin.type dst src */
  Instruction sin(Type type, RegisterIndex dst, RegisterIndex src);
  /*! tan.type dst src */
  Instruction tan(Type type, RegisterIndex dst, RegisterIndex src);
  /*! log.type dst src */
  Instruction log(Type type, RegisterIndex dst, RegisterIndex src);
  /*! sqr.type dst src */
  Instruction sqr(Type type, RegisterIndex dst, RegisterIndex src);
  /*! rsq.type dst src */
  Instruction rsq(Type type, RegisterIndex dst, RegisterIndex src);
  /*! pow.type dst src0 src1 */
  Instruction pow(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! mul.type dst src0 src1 */
  Instruction mul(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! add.type dst src0 src1 */
  Instruction add(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! sub.type dst src0 src1 */
  Instruction sub(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! div.type dst src0 src1 */
  Instruction div(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! rem.type dst src0 src1 */
  Instruction rem(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! shl.type dst src0 src1 */
  Instruction shl(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! shr.type dst src0 src1 */
  Instruction shr(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! asr.type dst src0 src1 */
  Instruction asr(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! bsf.type dst src0 src1 */
  Instruction bsf(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! bsb.type dst src0 src1 */
  Instruction bsb(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! or.type dst src0 src1 */
  Instruction or$(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! xor.type dst src0 src1 */
  Instruction xor$(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! and.type dst src0 src1 */
  Instruction and$(Type type, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! mad.type dst {src0, src1, src2} == src */
  Instruction mad(Type type, RegisterIndex dst, TupleIndex src);
  /*! cvt.{dstType <- srcType} dst src */
  Instruction cvt(Type dstType, Type srcType, RegisterIndex dst, RegisterIndex src0, RegisterIndex src1);
  /*! bra labelIndex */
  Instruction bra(LabelIndex labelIndex);
  /*! (pred) bra labelIndex */
  Instruction bra(LabelIndex labelIndex, RegisterIndex pred);
  /*! loadi.type dst value */
  Instruction loadi(Type type, RegisterIndex dst, ValueIndex value);
  /*! load.type.space {dst1,...,dst_valueNum} offset value */
  Instruction load(Type type, TupleIndex dst, RegisterIndex offset, MemorySpace space, uint16_t valueNum);
  /*! store.type.space offset {src1,...,src_valueNum} value */
  Instruction store(Type type, TupleIndex src, RegisterIndex offset, MemorySpace space, uint16_t valueNum);
  /*! fence.space */
  Instruction fence(MemorySpace space);
  /*! label labelIndex */
  Instruction label(LabelIndex labelIndex);

} /* namespace gbe */

#endif /* __GBE_IR_INSTRUCTION_HPP__ */

