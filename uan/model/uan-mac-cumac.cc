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

#include "uan-mac-cumac.h"
#include "uan-tx-mode.h"
#include "uan-address.h"
#include "uan-net-device.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "uan-phy.h"
#include "uan-header-common.h"
#include "ns3/random-variable.h"

#include <iostream>
NS_LOG_COMPONENT_DEFINE ("UanMacCumac");


namespace ns3 {

TonePulseTable m_pulseTable;


NS_OBJECT_ENSURE_REGISTERED (UanMacCumac);

UanMacCumac::UanMacCumac ()
  : UanMac (),
    m_status (IDLE),
    m_cleared (false)

{
}

UanMacCumac::~UanMacCumac ()
{
}

void
UanMacCumac::Clear ()
{
  if (m_cleared)
    {
      return;
    }
  m_cleared = true;
  if (m_phy)
    {
      m_phy->Clear ();
      m_phy = 0;
    }
}

void
UanMacCumac::DoDispose ()
{
  Clear ();
  UanMac::DoDispose ();
}

TypeId
UanMacCumac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UanMacCumac")
    .SetParent<Object> ()
    .AddConstructor<UanMacCumac> ()
  ;
  return tid;
}

Address
UanMacCumac::GetAddress (void)
{
  return m_address;
}

void
UanMacCumac::SetAddress (UanAddress addr)
{
  m_address=addr;
}
bool
UanMacCumac::Enqueue (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " Queueing packet for " << UanAddress::ConvertFrom (dest));
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " Accept?" << (m_hasPacket ? "NO" : "YES"));
  if (m_hasPacket)
    return false;

  NS_ASSERT(!m_phy->IsStateTx ());

  m_hasPacket = true;
  m_currentFrameNo++;
  m_packet = packet;
  m_protocolNumber = protocolNumber;
  m_dstAddress = UanAddress::ConvertFrom (dest);

  SendRts ();

  return true;
}


void
UanMacCumac::SendRts (void)
{
  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " " << m_address << " RTS " << m_dstAddress << "[frameNo=" << (int)m_currentFrameNo << "]");

  ChannelList channelList;

  m_channelManager.SetMobilityModel (GetMobilityModel ());
  for (uint32_t i = 1; i < m_modes.GetNModes (); i++) {
    if (m_channelManager.CanTransmit(Simulator::Now() + Seconds(1), i, GetPosition ()))
      channelList.insert (i);
  }

  for (uint32_t i = 1; i < m_modes.GetNModes() && channelList.size () < 3; i++) {
    channelList.insert (i);
  }

  std::stringstream str;
  ChannelList::iterator it = channelList.begin ();
  while (it != channelList.end ()) {
    int i = (*it); it++;
    str << " " << i;
  }

  NS_LOG_DEBUG("      AVAIL CHANNELS " << str.str ());


  UanHeaderCumacRts rts(m_currentFrameNo, m_packet->GetSize (), GetPosition (), channelList);
  rts.SetSrc (m_address);
  rts.SetDest (m_dstAddress);
  rts.SetType (RTS);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader(rts);

  SetChannel (0);
  m_status = SENDING_RTS;
  m_phy->SendPacket(packet, m_protocolNumber);

}

void
UanMacCumac::SetForwardUpCb (Callback<void, Ptr<Packet>, const UanAddress& > cb)
{
  m_forUpCb = cb;
}

void
LoadModesList (UanModesList &modes, uint16_t dataRateBps)
{
//  for (uint32_t i = 1; i <= 5; i++) {
//     uint32_t fcmode = 12000 + (i * dataRate) / 2.0;
//
//     UanTxMode mode = UanTxModeFactory::CreateMode (UanTxMode::QAM,
//                                                    dataRate,
//                                                    dataRate / 2,
//                                                    fcmode,
//                                                    dataRate,
//                                                    4,
//                                                    "channel " + i);
//     list.AppendMode(mode);
//
//     NS_LOG_INFO("[" << mode.GetUid() << "] [" << i << "] Mode: " << mode.GetModType()
//          << " FreqHz: " << mode.GetCenterFreqHz() << " BW: " << mode.GetBandwidthHz() << " Rate: " << mode.GetDataRateBps()
//          << " PhyRate: " << mode.GetPhyRateSps() << " CS: " << mode.GetConstellationSize());
//   }



  UanModesList list;
  for (uint8_t i = 0; i < 9; i++) {
     uint32_t fcmode = 10000 + (i * dataRateBps) / 2.0;

//     UanTxMode mode = UanTxModeFactory::CreateMode (UanTxMode::QAM,
//                                                    dataRateBps,
//                                                    dataRateBps / 2,
//                                                    fcmode,
//                                                    dataRateBps,
//                                                    4,
//                                                    "channel " + i);

//    UanTxMode mode = UanTxModeFactory::CreateMode (UanTxMode::FSK,
//                                                   dataRateBps,
//                                                   dataRateBps, 10000 + dataRateBps * i,
//                                                   dataRateBps, 2,
//                                                   "channel " + i);
     UanTxMode mode = UanTxModeFactory::CreateMode (UanTxMode::FSK,
                                                    dataRateBps,
                                                    dataRateBps,
                                                    fcmode,
                                                    dataRateBps, 2,
                                                    "channel " + i);

    list.AppendMode (mode);
  }

  modes = list;
}

void
UanMacCumac::AttachPhy (Ptr<UanPhy> phy)
{
  NS_LOG_DEBUG ("Attaching UanPhy to UanMacCumac (" << m_address << ")");

  m_phy = phy;
  m_phy->SetReceiveOkCallback (MakeCallback (&UanMacCumac::RxPacketGood, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&UanMacCumac::RxPacketError, this));
  m_phy->RegisterListener (this);

  LoadModesList (m_modes, 1000);
  NS_LOG_DEBUG ("No of modes: " << m_modes.GetNModes ());

  SetChannel (0);

//  m_channelManager.SetMobilityModel (GetMobilityModel ());
}

Address
UanMacCumac::GetBroadcast (void) const
{
  UanAddress broadcast (255);
  return broadcast;
}


void
UanMacCumac::RxPacketGood (Ptr<Packet> pkt, double sinr, UanTxMode txMode)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RxPacketGood");

  UanHeaderCommon header;
  pkt->PeekHeader (header);

  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " FROM " << header.GetSrc () << " TO " << header.GetDest () << " TYPE " << ((int) header.GetType ()));

  if (header.GetDest () == m_address) {
    if (header.GetType () == DATA) {
      NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RECEIVED DATA");

      NS_ASSERT(m_status == WAITING_DATA); // check frame_no
      NS_ASSERT(m_currentChannel != 0);

      m_waitDataEvent.Cancel ();
      m_forUpCb (pkt, header.GetSrc ());

      m_status = IDLE;
      SetChannel (0);
    } else if (header.GetType () == RTS) {
      NS_ASSERT(m_currentChannel == 0);

      UanHeaderCumacRts rts;
      pkt->RemoveHeader (rts);

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RTS RECEIVED [frameNo=" << (int)rts.GetFrameNo () << "]");

      StartBeacon (rts);
    } else if (header.GetType () == CTS) {
      UanHeaderCumacCts cts;
      pkt->RemoveHeader (cts);

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " CTS RECEIVED [frameNo=" << (int)cts.GetFrameNo () << ", channelNo=" << (int)cts.GetChannel ()  << "]");

      NS_ASSERT(cts.GetFrameNo () == m_currentFrameNo);
      NS_ASSERT(!m_phy->IsStateTx ());

      NS_ASSERT(m_hasPacket);

      m_waitCtsEvent.Cancel ();

      UanHeaderCumacData data (m_currentFrameNo);
      data.SetSrc (m_address);
      data.SetDest (m_dstAddress);
      data.SetType (DATA);

      m_packet->AddHeader(data);

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " SENDING DATA [frameNo=" << (int)cts.GetFrameNo () << ", channelNo=" << (int)cts.GetChannel ()  << ",length=" << m_packet->GetSize() << "]");

      m_status = SENDING_DATA;
      SetChannel (cts.GetChannel ());
      m_phy->SendPacket (m_packet, m_protocolNumber);
    }
  } else if (header.GetDest () == GetBroadcast ()) {
    if (header.GetType () == BEACON) {
      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RECEIVED BEACON FROM " << header.GetSrc ());
      UanHeaderCumacBeacon beacon;
      pkt->RemoveHeader (beacon);

      // use 'beacon response' wait time
      //Time delayToMe = CalculateDelay (beacon.GetDstPosition (), GetPosition ());

      m_channelManager.SetMobilityModel (GetMobilityModel ());
      if (!m_channelManager.CanTransmit(Simulator::Now () + Seconds (0.4),
              beacon.GetChannel (), beacon.GetDstPosition ())) {
        NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " NotifyBusy");
        m_pulseTable.NotifyBusy (beacon.GetChannel (), beacon.GetSignalInterval (), GetPosition ());
      }
    }
  } else {
    if (header.GetType () == CTS) {
      UanHeaderCumacCts cts;
      pkt->RemoveHeader (cts);

      DataRate dataRate(txMode.GetDataRateBps ());

      Time delayToMe = CalculateDelay(cts.GetDstPosition (), GetPosition ());
      Time delayToSrc = CalculateDelay(cts.GetDstPosition (), cts.GetSrcPosition ());

      Time ctsTxTime = Seconds (dataRate.CalculateTxTime (cts.GetSerializedSize ()));
      Time txStartTime = Simulator::Now() + (delayToSrc - delayToMe) + ctsTxTime;

      Time dataTxTime = Seconds (dataRate.CalculateTxTime (cts.GetPacketSize() + 1));

      m_channelManager.SetMobilityModel (GetMobilityModel ());
      m_channelManager.RegisterTransmission(txStartTime, cts.GetChannel (),
              cts.GetSrcPosition (), dataTxTime);
    }
  }


//  NS_LOG_DEBUG ("Receiving packet from " << header.GetSrc () << " For " << header.GetDest ());
//
//  if (header.GetDest () == GetAddress () || header.GetDest () == UanAddress::GetBroadcast ())
//    {
//      m_forUpCb (pkt, header.GetSrc ());
//    }

}

void
UanMacCumac::RxPacketError (Ptr<Packet> pkt, double sinr)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << UanAddress::ConvertFrom (GetAddress ()) << " Received packet in error with sinr " << sinr);
}


void
UanMacCumac::NotifyRxStart (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RxStart");
}

void
UanMacCumac::NotifyRxEndOk (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RxEndOk");
}

void
UanMacCumac::NotifyRxEndError (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " RxEndError");
}

void
UanMacCumac::NotifyCcaStart (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " CcaStart");
}

void
UanMacCumac::NotifyCcaEnd (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " CcaEnd");
}

void
UanMacCumac::NotifyTxStart (Time duration)
{
  NS_ASSERT(!m_tx);
  m_tx = true;

  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " TxStart [status=" << m_status << ", duration=" << duration.GetSeconds () << "]");

  Simulator::Schedule (duration, &UanMacCumac::EndTx, this);
}

void
UanMacCumac::EndTx (void)
{
  NS_ASSERT(m_tx);
  m_tx = false;

  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " TxEnd [status=" << m_status << "]");

  switch (m_status) {
    case SENDING_BEACON:
      WaitBeacon ();
      break;

    case SENDING_CTS:
      WaitData ();
      break;

    case SENDING_RTS:
      WaitCts ();
      break;

    case SENDING_DATA:
      m_status = IDLE;
      m_hasPacket = false;
      SetChannel (0);
      break;
    default:
      NS_ASSERT(false);
  }
}

void
UanMacCumac::SetChannel (uint8_t channel)
{
  m_currentChannel = channel;

  UanModesList list;
  list.AppendMode (m_modes[channel]);

  m_phy->SetAttribute ("SupportedModes", UanModesListValue (list));
}


Ptr<MobilityModel>
UanMacCumac::GetMobilityModel (void) const
{
  Ptr<NetDevice> device = m_phy->GetDevice ();
  Ptr<Node> node = device->GetNode ();
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();

  return mobility;
}

Vector
UanMacCumac::GetPosition (void) const
{
  return GetMobilityModel ()->GetPosition();
}

Time
UanMacCumac::CalculateDelay (Vector a, Vector b) const
{
  return Seconds (CalculateDistance (a, b) / 1500.0);
}

void
UanMacCumac::StartBeacon (UanHeaderCumacRts &rts)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " START BEACON ");

  NS_ASSERT(m_currentChannel == 0);
  NS_ASSERT(!m_phy->IsStateTx ());

  Time maxRtt = Seconds (2 * 550 / 1500.0);

  m_channelsToTry.clear ();

  m_rtsReceived = rts;
  const ChannelList &rtsChannels = rts.GetChannelList ();

  m_channelManager.SetMobilityModel (GetMobilityModel ());
  ChannelList::const_iterator it = rtsChannels.begin ();
  for ( ; it != rtsChannels.end(); it++) {
    if (m_channelManager.CanTransmit(Simulator::Now() + maxRtt, *it, GetPosition ()))
      m_channelsToTry.insert (*it);
  }

  it = rtsChannels.begin ();
  for ( ; m_channelsToTry.size () < 3 && it != rtsChannels.end (); it++) {
    m_channelsToTry.insert (*it);
  }

  m_currentTryingChannel = m_channelsToTry.begin ();
  SendBeacon ();
}

void
UanMacCumac::SendBeacon (void)
{
  if (m_currentTryingChannel == m_channelsToTry.end ()) {
    NS_ASSERT(m_status == IDLE || m_status == WAITING_BEACON_RESPONSE);
    m_status = IDLE;
    return;
  }

  UniformVariable uv;

  m_signalInterval = uv.GetInteger(0, 11);

  UanHeaderCumacBeacon beacon (*m_currentTryingChannel, m_signalInterval, GetPosition ());
  beacon.SetSrc (m_address);
  beacon.SetDest (UanAddress (255));
  beacon.SetType (BEACON);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (beacon);

  m_status = SENDING_BEACON;
  m_phy->SendPacket(packet, m_protocolNumber);
}

void
UanMacCumac::WaitBeacon (void)
{
  m_status = WAITING_BEACON_RESPONSE;
  m_beaconCheckTimes = 0;

  CheckBeacon ();
}

void
UanMacCumac::CheckBeacon (void)
{
  if (m_pulseTable.IsBusy(*m_currentTryingChannel, m_signalInterval, GetPosition ())) {
    NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " CHANNEL " << ((int) *m_currentTryingChannel) << " IS BUSY");
    m_currentTryingChannel++;
    SendBeacon ();
  } else if (m_beaconCheckTimes++ < 4) {
    NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " CHECKING FOR TONE PULSE...");
    Simulator::Schedule(Seconds(0.2), &UanMacCumac::CheckBeacon, this);
  } else {
    NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " NO TONE PULSE RECEIVED");
    StartCts ();
  }
}

void
UanMacCumac::StartCts (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " SENDING CTS");
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " CHANNEL: " << ((int)*m_currentTryingChannel));

  NS_ASSERT(m_currentChannel == 0);
  NS_ASSERT(m_status = WAITING_BEACON_RESPONSE);

  UanHeaderCumacCts cts(*m_currentTryingChannel, m_rtsReceived.GetFrameNo(),
                        m_rtsReceived.GetLength (),
                        m_rtsReceived.GetPosition(), GetPosition ());
  cts.SetSrc (m_address);
  cts.SetDest (m_rtsReceived.GetSrc ());
  cts.SetType (CTS);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader(cts);

  m_status = SENDING_CTS;
  m_phy->SendPacket (packet, m_protocolNumber);
}

void
UanMacCumac::WaitData (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " WAITING FOR DATA [frameNo=" << (int) m_rtsReceived.GetFrameNo () << ", channelNo=" << (int) *m_currentTryingChannel << "]");

  m_status = WAITING_DATA;
  SetChannel (*m_currentTryingChannel);

  Time delay = CalculateDelay (GetPosition (), m_rtsReceived.GetPosition ());

  DataRate dataRate (m_modes[m_currentChannel].GetDataRateBps ());
  Time txDelay = Seconds (dataRate.CalculateTxTime (m_rtsReceived.GetLength ()));

  Time totalDelay = delay + delay + txDelay + Seconds(0.1);

  NS_LOG_DEBUG ("ESTIMATED WAITING DELAY: [txDelay=" << txDelay.GetSeconds() << "s, propDelay=" << (delay + delay).GetSeconds () << "s, totalDelay=" << totalDelay.GetSeconds () << "s]");


  m_waitDataEvent = Simulator::Schedule(totalDelay, &UanMacCumac::CancelWaitData, this);
}

void
UanMacCumac::CancelWaitData (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " STOPED WAITING FOR DATA");
  m_status = IDLE;
  SetChannel (0);
}

void
UanMacCumac::WaitCts (void)
{
  m_status = WAITING_CTS;

  m_waitCtsEvent = Simulator::Schedule(Seconds (2), &UanMacCumac::CancelWaitCts, this);
}

void
UanMacCumac::CancelWaitCts (void)
{
  // packet dropped
  m_status = IDLE;

  //SendRts ();
}

}

namespace ns3
{

TonePulseTable::TonePulseTable ()
{

}

TonePulseTable::~TonePulseTable()
{

}

TypeId
TonePulseTable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TonePulseTable")
    .SetParent<Object> ()
    .AddConstructor<TonePulseTable> ()
  ;
  return tid;

}


void
TonePulseTable::NotifyBusy(uint8_t channel, uint8_t interval, Vector position)
{
  Entry e;
  e.m_channel = channel;
  e.m_interval = interval;
  e.m_position = position;

  uint32_t id = ++m_nextId;

  Simulator::Schedule(Seconds (0.4), &TonePulseTable::Remove, this, id);

  m_entries[id] = e;
}

void
TonePulseTable::Remove (uint32_t id)
{
  m_entries.erase (id);
}

bool
TonePulseTable::IsBusy(uint8_t channel, uint8_t interval, Vector position) const
{
  std::map<uint32_t, Entry>::const_iterator it = m_entries.begin ();
  for ( ; it != m_entries.end (); it++) {
    const Entry &e = it->second;

    if (e.m_channel != channel || e.m_interval != interval)
      continue;

    if (CalculateDistance (e.m_position, position) < 550)
      return true;
  }

  return false;
}

}
