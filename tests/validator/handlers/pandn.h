// Copyright 2013-2019 Stanford University
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

namespace stoke {

class ValidatorPandnTest : public StraightLineValidatorTest {};

TEST_F(ValidatorPandnTest, Identity) {

  target_ << ".foo:" << std::endl;
  target_ << "pandn %xmm3, %xmm5" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << ".foo:" << std::endl;
  rewrite_ << "pandn %xmm3, %xmm5" << std::endl;
  rewrite_ << "retq" << std::endl;

  assert_equiv();

}

TEST_F(ValidatorPandnTest, NotANoop) {

  target_ << ".foo:" << std::endl;
  target_ << "pandn %xmm3, %xmm5" << std::endl;
  target_ << "retq" << std::endl;

  rewrite_ << ".foo:" << std::endl;
  rewrite_ << "retq" << std::endl;

  assert_ceg();

}

TEST_F(ValidatorPandnTest, LooksCorrect) {

  target_ << ".foo:" << std::endl;
  target_ << "pandn %xmm3, %xmm5" << std::endl;
  target_ << "retq" << std::endl;

  Sandbox sb;
  sb.set_abi_check(false);
  StateGen sg(&sb);
  CpuState cs;
  sg.get(cs);

  check_circuit(cs);

}

} //namespace stoke
