// Copyright 2013-2015 Eric Schkufza, Rahul Sharma, Berkeley Churchill, Stefan Heule
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


class ValidatorMemoryTest : public ValidatorTest { };

TEST_F(ValidatorMemoryTest, ReadAfterSameWrite) {

  target_ << "movq %rdx, (%rsp)" << std::endl;
  target_ << "movq (%rsp), %rax" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << "movq %rdx, %rax" << std::endl;
  rewrite_ << "retq" << std::endl;

  assert_equiv();

}

TEST_F(ValidatorMemoryTest, Read32AfterWrite64) {

  target_ << "movq %rdx, (%rsp)" << std::endl;
  target_ << "movl (%rsp), %eax" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << "movl %edx, %eax" << std::endl;
  rewrite_ << "retq" << std::endl;

  set_live_outs(x64asm::RegSet::empty() + x64asm::rax + x64asm::rdx);
  assert_equiv();
}

TEST_F(ValidatorMemoryTest, Read16AfterBiggerWrite64) {

  target_ << "movq %rdx, (%rsp)" << std::endl;
  target_ << "movw (%rsp), %ax" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << "movw %dx, %ax" << std::endl;
  rewrite_ << "retq" << std::endl;

  set_live_outs(x64asm::RegSet::empty() + x64asm::rax + x64asm::rdx);
  assert_equiv();
}

TEST_F(ValidatorMemoryTest, Read8AfterBiggerWrite64) {

  target_ << "movq %rdx, (%rsp)" << std::endl;
  target_ << "movb (%rsp), %al" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << "movb %dl, %al" << std::endl;
  rewrite_ << "retq" << std::endl;

  set_live_outs(x64asm::RegSet::empty() + x64asm::rax + x64asm::rdx);
  assert_equiv();
}

TEST_F(ValidatorMemoryTest, Read16AfterTwoWrite8) {

  // MEMORY LOOKS LIKE:
  // | DL | CL |
  //   ^
  //  RSP
  // (small)

  // RAX LOOKS LIKE:
  // | CL | DL |

  target_ << "movb %dl, (%rsp)" << std::endl;
  target_ << "movb %cl, 0x1(%rsp)" << std::endl;
  target_ << "movw (%rsp), %ax" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << "movb %cl, %al"  << std::endl;
  rewrite_ << "shlw $0x8, %ax" << std::endl;
  rewrite_ << "movb %dl, %al"  << std::endl;
  rewrite_ << "retq" << std::endl;

  set_live_outs(x64asm::RegSet::empty() + x64asm::rax + x64asm::rdx);
  assert_equiv();
}
