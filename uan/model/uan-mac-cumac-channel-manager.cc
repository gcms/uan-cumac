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

#include "ns3/log.h"

#include "uan-mac-cumac-channel-manager.h"

NS_LOG_COMPONENT_DEFINE ("UanMacCumacChannelManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED ( UanMacCumacChannelManager);

UanMacCumacChannelManager::UanMacCumacChannelManager () :
  Object ()
{
}


UanMacCumacChannelManager::~UanMacCumacChannelManager ()
{
}

void
UanMacCumacChannelManager::DoDispose ()
{
  Object::DoDispose ();
}

TypeId
UanMacCumacChannelManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UanMacCumacChannelManager").
          SetParent<Object> ().
          AddConstructor<UanMacCumacChannelManager> ();
  return tid;
}

void
UanMacCumacChannelManager::SetMobilityModel (Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

/*
 * start: time that the sender will start to tx
 * channelNo: channel used to tx
 * srcPosition: position of the sender
 * dstPosition: position of the receiver
 * txDelay: transmission time for the packet (propagation delay not included)
 */
void
UanMacCumacChannelManager::RegisterTransmission (Time start, uint8_t channelNo,
                                                 Vector srcPosition, Time txDelay)
{
  m_transmissions.push_back (Entry (channelNo, srcPosition, start, txDelay));
}

/*
 * check if channelNo will be free to transmit to dstPosition at a given time
 */
bool
UanMacCumacChannelManager::CanTransmit (Time time, uint8_t channelNo, Vector dstPosition)
{
//  std::cout << "CanTransmit " << m_transmissions.size () << std::endl;

  EntryList::iterator it = m_transmissions.begin ();
  for (; it != m_transmissions.end (); it++) {
    Entry &entry = *it;

    if (entry.IsExpired (time)) {
      std::cout << entry.GetSrcPosition () << " expired" << std::endl;
      m_transmissions.erase (it);
    }
  }

  it = m_transmissions.begin ();
  for (; it != m_transmissions.end(); it++) {
    Entry &entry = *it;

    std::cout << channelNo << " <=> " << entry.GetChannel () << std::endl;
    if (channelNo != entry.GetChannel ())
      continue;

    std::cout << "Distance" << CalculateDistance (entry.GetSrcPosition (), dstPosition) << std::endl;
    if (CalculateDistance (entry.GetSrcPosition (), dstPosition) > 550)
      continue;

    std::cout << "CurrentTime " << time << std::endl;
    std::cout << "FinishTime " << entry.CalculateFinishTime (dstPosition) << std::endl;
    if (time >= entry.GetStartTime () && time <= entry.CalculateFinishTime (dstPosition))
      return false;
  }

  return true;
}

}

