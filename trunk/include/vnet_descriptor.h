/*
* ipop-project
* Copyright 2016, University of Florida
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
#ifndef TINCAN_VNET_DESCRIPTOR_H_
#define TINCAN_VNET_DESCRIPTOR_H_
#include "tincan_base.h"
namespace tincan
{
struct VnetDescriptor
{
  string uid;
  string name;
  string mac;
  string vip4;
  uint32_t prefix4;
  uint32_t mtu4;
  string vip6;
  uint32_t prefix6;
  uint32_t mtu6;
  string description;
  string stun_addr;
  string turn_addr;
  string turn_user;
  string turn_pass;
  bool l2tunnel_enabled;
  bool auto_trim_enabled;
  bool address_translation_enabled;
  struct in_addr vip4_addr;
  struct in6_addr vip6_addr;
};
} // namespace tincan
#endif // TINCAN_VNET_DESCRIPTOR_H_