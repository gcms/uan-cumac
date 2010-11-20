/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "ns3/uan-header-cumac.h"
#include "ns3/test.h"


using namespace ns3;

class UanHeaderCumacTest : public TestCase
{
public:
  UanHeaderCumacTest ();

  virtual bool DoRun (void);
};


UanHeaderCumacTest::UanHeaderCumacTest () : TestCase ("UanHeaderCumacTest")
{
}


bool
UanHeaderCumacTest::DoRun (void)
{
  ChannelList list;
  list.insert (1);
  list.insert (2);

  UanHeaderCumacRts send(10, 300, Vector3D(255, 256, 257), list);
  send.SetSrc (UanAddress (0));
  send.SetDest (UanAddress (1));
  send.SetType (1);

  Buffer buffer;
  buffer.AddAtStart (send.GetSerializedSize ());
  send.Serialize(buffer.Begin ());

  UanHeaderCumacRts receive;
  receive.Deserialize (buffer.Begin ());
  NS_TEST_ASSERT_MSG_EQ (2, receive.GetChannelList ().size (),
                         "Should have 2 channels");

  NS_TEST_ASSERT_MSG_EQ (1, receive.GetDest (), "destination should be 1");

  NS_TEST_ASSERT_MSG_EQ (255, receive.GetPosition ().x, "X is wrong");
  NS_TEST_ASSERT_MSG_EQ (256, receive.GetPosition ().y, "Y is wrong");
  NS_TEST_ASSERT_MSG_EQ (257, receive.GetPosition ().z, "Z is wrong");



  UanHeaderCumacCts sendCts(2, 3, 300, Vector3D (255, 256, 257), Vector3D (255, 256, 257));
  sendCts.SetSrc (UanAddress (1));
  sendCts.SetDest (UanAddress (2));
  sendCts.SetType (3);

  Buffer buffer2;
  buffer2.AddAtStart (sendCts.GetSerializedSize ());
  sendCts.Serialize(buffer2.Begin ());

//  Buffer::Iterator it = buffer2.Begin ();
//  while (!it.IsEnd ())
//    std::cout << (int) it.ReadU8 () << std::endl;

  UanHeaderCumacCts receiveCts;
  receiveCts.Deserialize (buffer2.Begin ());

  NS_TEST_ASSERT_MSG_EQ (2, receiveCts.GetDest (), "destination should be 1");

//  std::cout << (int) sendCts.GetChannel () << std::endl;
//  std::cout << (int) receiveCts.GetChannel () << std::endl;

  NS_TEST_ASSERT_MSG_EQ (3, receiveCts.GetFrameNo (),
                           "Should be frameNo 3");


  NS_TEST_ASSERT_MSG_EQ (2, receiveCts.GetChannel (),
                         "Should be channel 2");









  return GetErrorStatus ();

}


class UanHeaderCumacTestSuite : public TestSuite
{
public:
  UanHeaderCumacTestSuite ();
};

UanHeaderCumacTestSuite::UanHeaderCumacTestSuite ()
  :  TestSuite ("uan-header-cumac", UNIT)
{
  AddTestCase (new UanHeaderCumacTest);
}

UanHeaderCumacTestSuite g_uanHeaderCumacSuite;




