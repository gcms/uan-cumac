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

#include "ns3/uan-mac-cumac-channel-manager.h"
#include "ns3/test.h"


using namespace ns3;

class UanMacCumacTest : public TestCase
{
public:
  UanMacCumacTest ();

  virtual bool DoRun (void);
};


UanMacCumacTest::UanMacCumacTest () : TestCase ("UanMacCumacTest")
{

}


bool
UanMacCumacTest::DoRun (void)
{
  Time start = Seconds (0);
  Vector3D n1 = Vector3D (0, 0, 0);
  Vector3D n2 = Vector3D (0, 0, 300);
  Vector3D n3 = Vector3D (0, 300, 300);

  UanMacCumacChannelManager channelMan;

  channelMan.RegisterTransmission (start, 1, n1, Seconds (300.0 / 1000.0));
  NS_TEST_ASSERT_MSG_EQ (false, channelMan.CanTransmit (Seconds (0.3), 1, n3),
                         "Should be false");
  NS_TEST_ASSERT_MSG_EQ (true, channelMan.CanTransmit (Seconds (0.6), 1, n3),
                         "Should be true");

  return GetErrorStatus ();

}


class UanMacCumacTestSuite : public TestSuite
{
public:
  UanMacCumacTestSuite ();
};

UanMacCumacTestSuite::UanMacCumacTestSuite ()
  :  TestSuite ("uan-mac-cumac", UNIT)
{
  AddTestCase (new UanMacCumacTest);
}

UanMacCumacTestSuite g_uanMacCumacSuite;




