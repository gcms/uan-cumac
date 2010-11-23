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

UniformVariable m_uv;


NS_OBJECT_ENSURE_REGISTERED (UanMacCumac);

UanMacCumac::UanMacCumac ()
  : UanMac (),
    m_maxPropDelay (Seconds (550.0 / 1500.0)),
    m_cwMin (2),
    m_cwMax (8),
    m_tryingRts (false),
    m_status (IDLE),
    m_tx (false),
    m_cleared (false)
{
  m_cw = m_cwMin;
  m_timeSlot = Seconds (m_maxPropDelay.GetSeconds () / 2.0);//std::pow(2, (double) m_cwMin));
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
  m_address = addr;
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

  StartRts ();

  return true;
}

void
UanMacCumac::StartRts ()
{
  m_numRetries = 0;
  m_cw = std::max(m_cwMin, m_cw - 1);

  TryRts ();
}

void
UanMacCumac::TryRts ()
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " TRYING RTS TO " << m_dstAddress);

  NS_ASSERT(m_status == IDLE);
  if (++m_numRetries > 3) // maximum retries
    return;

  int timeSlots = m_uv.GetInteger(0, std::pow((double)2, m_cw));
  m_cw = std::min(m_cwMax, m_cw + 1);

  m_timeCurrentDelay = m_maxPropDelay + Seconds (m_timeSlot.GetSeconds () * timeSlots);

  m_tryingRts = true;
  m_timerRunning = false;
  if (m_phy->IsStateIdle ()) {
    StartTimer ();
  }
}

void
UanMacCumac::StartTimer (void)
{
  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " StartTimer()");

  NS_ASSERT(m_currentChannel == 0);
  NS_ASSERT(m_tryingRts);
  NS_ASSERT(!m_timerRunning);
  NS_ASSERT(m_rtsCts || m_phy->IsStateIdle()); // either was called from cts timer or from channel idle

  if (m_rtsCts) {
    m_rtsCts = false;
  }

  if (!m_phy->IsStateIdle ())
    return;

  m_timeStartDelay = Simulator::Now ();
//  m_timeCurrentDelay = std::max(m_timeCurrentDelay, m_maxPropDelay);
  m_currentTimer = Simulator::Schedule(m_timeCurrentDelay, &UanMacCumac::SendRts, this);
  m_timerRunning = true;

  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " Starting timer... starts at: " << (Simulator::Now() + m_timeCurrentDelay) << "s");
}

void
UanMacCumac::StopTimer (void)
{
  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " StopTimer()");

  NS_ASSERT(m_currentChannel == 0);
  NS_ASSERT(m_tryingRts);
  NS_ASSERT(m_timerRunning);

  m_currentTimer.Cancel ();
  m_timeCurrentDelay = m_timeCurrentDelay - (Simulator::Now () - m_timeStartDelay);
  m_timerRunning = false;

  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " Stopping timer... still need: " << (m_timeCurrentDelay) << "s");
}


void
UanMacCumac::SendRts (void)
{
  m_tryingRts = false;

  NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RTS " << m_dstAddress << "[frameNo=" << (int)m_currentFrameNo << "]");

  ChannelList channelList;

  DataRate dataRate(m_modes[m_currentChannel].GetDataRateBps ());

  // time for rts + beacon (2 way) + cts
  Time estimatedStartTime = Simulator::Now () + CalculateDelay (m_address, m_dstAddress) // rts prop
          + m_maxPropDelay + m_maxPropDelay // beacon prop
          + CalculateDelay (m_address, m_dstAddress); // cts
  Time estimatedFinishTime = estimatedStartTime + Seconds (dataRate.CalculateTxTime (m_packet->GetSize()))
          + CalculateDelay (m_address, m_dstAddress) + Seconds (0.2);

  m_channelManager.SetMobilityModel (GetMobilityModel ());

  for (uint8_t i = 1; i < m_modes.GetNModes (); i++) {
    if (!m_channelManager.IsRegistered(i, GetPosition ()))
        channelList.insert (i);
    }

  for (uint8_t i = 1; i < m_modes.GetNModes () && channelList.size () < 3; i++) {
    if (m_channelManager.CanTransmitFromSrc(i, estimatedStartTime, estimatedFinishTime, GetPosition ()))
      channelList.insert (i);
  }

  for (uint8_t i = 1; i < m_modes.GetNModes() && channelList.size () < 3; i++) {
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
LoadModesList (UanModesList &modes, UanTxMode baseMode)
{
  uint32_t centerFreq = baseMode.GetCenterFreqHz ();
  uint32_t dataRateBps = baseMode.GetDataRateBps ();
  uint32_t constSize = baseMode.GetConstellationSize ();

  UanModesList list;
  for (uint8_t i = 0; i < 9; i++) {
     uint32_t fcmode = centerFreq + (i * dataRateBps * 2) / 2.0;

     UanTxMode mode = UanTxModeFactory::CreateMode (UanTxMode::FSK,
                                                    dataRateBps,
                                                    dataRateBps,
                                                    fcmode,
                                                    dataRateBps, constSize,
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

  UanModesListValue list;
  m_phy->GetAttribute("SupportedModes", list);

  LoadModesList (m_modes, list.Get () [0]);
  NS_LOG_DEBUG ("No of modes: " << m_modes.GetNModes ());

  SetChannel (0);
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

  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " FROM " << header.GetSrc () << " TO " << header.GetDest () << " TYPE " << ((int) header.GetType ()));

  if (header.GetDest () == m_address || header.GetDest() == GetBroadcast ()) {
    if (header.GetType () == DATA) {
      NS_LOG_DEBUG("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RECEIVED DATA");

      NS_ASSERT(m_currentChannel != 0);
      NS_ASSERT(m_status == WAITING_DATA); // check frame_no

      m_waitDataEvent.Cancel ();
      m_forUpCb (pkt, header.GetSrc ());

      m_status = IDLE;
      SetChannel (0);
    } else if (header.GetType () == RTS) {
      NS_ASSERT(m_currentChannel == 0);

      UanHeaderCumacRts rts;
      pkt->RemoveHeader (rts);
      RegisterPosition (rts.GetSrc (), rts.GetPosition ());

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RTS RECEIVED [frameNo=" << (int)rts.GetFrameNo () << "]");

      StartBeacon (rts);
    } else if (header.GetType () == CTS) {
      UanHeaderCumacCts cts;
      pkt->RemoveHeader (cts);
      RegisterPosition (cts.GetSrc (), cts.GetSrcPosition());
      RegisterPosition (cts.GetDest (), cts.GetDstPosition());

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

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " SENDING DATA TO " << m_dstAddress << " [frameNo=" << (int)cts.GetFrameNo () << ", channelNo=" << (int)cts.GetChannel ()  << ",length=" << m_packet->GetSize() << "]");

      m_status = SENDING_DATA;
      SetChannel (cts.GetChannel ());
      m_phy->SendPacket (m_packet, m_protocolNumber);
    } else if (header.GetType () == BEACON) {
      UanHeaderCumacBeacon beacon;
      pkt->RemoveHeader (beacon);
      RegisterPosition (beacon.GetSrc (), beacon.GetSrcPosition());
      RegisterPosition (beacon.GetDest (), beacon.GetDstPosition());

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RECEIVED BEACON FROM " << header.GetSrc () << " CHANNEL: " << (int) beacon.GetChannel ());
      // use 'beacon response' wait time
//      Time delayToMe = CalculateDelay (beacon.GetDstPosition (), GetPosition ());

      Time tonePulseInterval =  Seconds ((12 + 4 * beacon.GetSignalInterval ()) / 1000.0);

      DataRate dataRate (m_modes[m_currentChannel].GetDataRateBps ());

      // Beacon response + cts
      Time estimatedStartTime = Simulator::Now () + m_maxPropDelay + CalculateDelay (beacon.GetSrcPosition (), beacon.GetDstPosition ());
      Time estimatedFinishTime = estimatedStartTime + tonePulseInterval + Seconds (dataRate.CalculateTxTime (beacon.GetLength ()));

      m_channelManager.SetMobilityModel (GetMobilityModel ());
      if (!m_channelManager.CanTransmit(beacon.GetChannel (), estimatedStartTime, estimatedFinishTime,
              beacon.GetSrcPosition(), beacon.GetDstPosition ())) {
        NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " NotifyBusy");
        m_pulseTable.NotifyBusy (beacon.GetChannel (), beacon.GetSignalInterval (), GetPosition ());
      } else {
        //If an item in the channel usage table corresponding to the receiver i has been in the reserved status for more than 2T + nτi
        Time start = Simulator::Now ();
        Time finish = start + m_maxPropDelay + m_maxPropDelay + tonePulseInterval + CalculateDelay (beacon.GetDstPosition(), GetPosition ());

        NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " !!!! REGISTERED Tx [" << (int)beacon.GetChannel () << "] "  << start.GetSeconds () << " " << finish.GetSeconds ());
        m_channelManager.RegisterTransmission (beacon.GetChannel (), start, finish,
                beacon.GetSrcPosition (), beacon.GetDstPosition ());
      }
    }
  } else {
    if (header.GetType () == CTS) {
      UanHeaderCumacCts cts;
      pkt->RemoveHeader (cts);
      RegisterPosition (cts.GetSrc (), cts.GetSrcPosition());
      RegisterPosition (cts.GetDest (), cts.GetDstPosition());

      DataRate dataRate(txMode.GetDataRateBps ());

      Time delayToMe = CalculateDelay(cts.GetDstPosition (), GetPosition ());
      Time delayToSrc = CalculateDelay(cts.GetDstPosition (), cts.GetSrcPosition ());

      Time ctsTxTime = Seconds (dataRate.CalculateTxTime (cts.GetSerializedSize ()));

      Time txStartTime = Simulator::Now () + (delayToSrc - delayToMe);
      Time dataTxTime = Seconds (dataRate.CalculateTxTime (cts.GetPacketSize() + 1)) + ctsTxTime;

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " !!!! REGISTERED Tx [" << (int)cts.GetChannel () << "] " << txStartTime.GetSeconds () << " " << (txStartTime + dataTxTime).GetSeconds ());
      m_channelManager.SetMobilityModel (GetMobilityModel ());
      m_channelManager.RegisterTransmission(cts.GetChannel (), txStartTime, txStartTime + dataTxTime,
              cts.GetSrcPosition (), cts.GetDstPosition ());

      if (m_tryingRts && m_rtsCts) {
        m_rtsWaitCtsEvent.Cancel ();

        StartTimer();
      }
    } else if (header.GetType () == RTS && m_tryingRts) {
      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RTS WHILE TRYING TO RTS");
      NS_ASSERT(m_currentChannel == 0);

      UanHeaderCumacRts rts;
      pkt->RemoveHeader (rts);
      RegisterPosition (rts.GetSrc (), rts.GetPosition());

      Time delayToMe = CalculateDelay (rts.GetPosition (), GetPosition ());
      Time delayDstSrc = CalculateDelay (rts.GetSrc (), rts.GetDest ());
      Time timeToWait = (delayDstSrc - delayToMe) + delayDstSrc + m_maxPropDelay + m_maxPropDelay;

      if (m_timerRunning)
        StopTimer ();

      if (m_rtsCts)
        m_rtsWaitCtsEvent.Cancel ();

      NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " Starting Timer at " << (Simulator::Now () + timeToWait).GetSeconds ());
      m_rtsCts = true;
      m_rtsWaitCtsEvent = Simulator::Schedule(timeToWait, &UanMacCumac::StartTimer, this);
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
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << UanAddress::ConvertFrom (GetAddress ()) << " Received packet in error with sinr " << sinr << "[channel=" << (int) m_currentChannel << "]");
}


void
UanMacCumac::NotifyRxStart (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RxStart");

  if (m_tryingRts && !m_rtsCts && m_timerRunning) {
    StopTimer ();
  }
}

void
UanMacCumac::NotifyRxEndOk (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " RxEndOk " << m_phy->IsStateBusy ());

  if (m_tryingRts && !m_rtsCts && !m_phy->IsStateBusy () && !m_timerRunning) {
    StartTimer ();
  }
}

void
UanMacCumac::NotifyRxEndError (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " RxEndError " << m_phy->IsStateBusy ());

  if (m_tryingRts && !m_rtsCts && !m_phy->IsStateBusy () && !m_timerRunning) {
    StartTimer ();
  }
}

void
UanMacCumac::NotifyCcaStart (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " CcaStart");
  if (m_tryingRts && !m_rtsCts && m_timerRunning) {
    StopTimer ();
  }
}

void
UanMacCumac::NotifyCcaEnd (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now () << " MAC " << m_address << " CcaEnd");
  if (m_tryingRts && !m_rtsCts && !m_phy->IsStateBusy () && !m_timerRunning) {
    StartTimer ();
  }
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

  m_signalInterval = m_uv.GetInteger(0, 12);

  Time maxRtt = m_maxPropDelay + m_maxPropDelay;
  Time tonePulseInterval =  Seconds ((12 + 4 * m_signalInterval) / 1000.0);

  /* Selecting possible channels... */
  m_channelsToTry.clear ();

  m_rtsReceived = rts;
  const ChannelList &rtsChannels = rts.GetChannelList ();

  DataRate dataRate (m_modes[m_currentChannel].GetDataRateBps ());
  Time txDelay = Seconds (dataRate.CalculateTxTime (m_rtsReceived.GetLength ()));

  // beacon (2 way) + cts response
  Time estimatedStartTime = Simulator::Now () + m_maxPropDelay + m_maxPropDelay + tonePulseInterval + m_maxPropDelay;
  Time estimatedFinishTime = estimatedStartTime + m_maxPropDelay + txDelay + Seconds (0.1);

  m_channelManager.SetMobilityModel (GetMobilityModel ());
  ChannelList::const_iterator it = rtsChannels.begin ();
  for ( ; it != rtsChannels.end(); it++) {
    if (m_channelManager.CanTransmit(*it, estimatedStartTime, estimatedFinishTime,
            m_rtsReceived.GetPosition (), GetPosition ()))
      m_channelsToTry.insert (*it);
  }

  it = rtsChannels.begin ();
  for ( ; m_channelsToTry.size () < 3 && it != rtsChannels.end (); it++) {
    m_channelsToTry.insert (*it);
  }
  /* End selecting possible channels... TODO: extract this to another method*/

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

  UanHeaderCumacBeacon beacon (*m_currentTryingChannel, m_signalInterval, m_rtsReceived.GetLength (),
                               m_rtsReceived.GetPosition (), GetPosition ());
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
  } else if (m_beaconCheckTimes++ < 2) {
    NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " CHECKING FOR TONE PULSE...");
    double totalTxRxTimeInSeconds = (2 * 550.0 / 1500.0 + (12 + 4 * m_signalInterval) / 1000.0); // 2T + n*tau_i

    Simulator::Schedule(Seconds(totalTxRxTimeInSeconds / 2.0), &UanMacCumac::CheckBeacon, this);
  } else {
    NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " NO TONE PULSE RECEIVED");
    StartCts ();
  }
}

void
UanMacCumac::StartCts (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " SENDING CTS TO " << m_rtsReceived.GetSrc () );
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

  //After a receiver sends its CTS to the sender and switches to the selected data channel, it will start a timer which will expire after 2T.
  Time propDelay = CalculateDelay (GetPosition (), m_rtsReceived.GetPosition ());
  DataRate dataRate (m_modes[m_currentChannel].GetDataRateBps ());
  Time txDelay = Seconds (dataRate.CalculateTxTime (m_rtsReceived.GetLength () + 10));

  // 2*propDelay + txDelay
  Time totalDelay = propDelay + propDelay + txDelay + Seconds (0.1);

  NS_LOG_DEBUG ("ESTIMATED WAITING DELAY: [txDelay=" << txDelay.GetSeconds() << "s, propDelay=" << (propDelay + propDelay).GetSeconds () << "s, totalDelay=" << totalDelay.GetSeconds () << "s]");


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
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " WAITING CTS");
  m_rtsCts = false;

  m_status = WAITING_CTS;

  Time propDelaySrcDst = CalculateDelay (m_address, m_dstAddress);

  // Enough time for propagation of rts + propagation of three beacons + propagation of cts
  // also an additional time for accounting the tx Delay
  Time totalDelay = propDelaySrcDst + propDelaySrcDst // rts + cts
          + m_maxPropDelay + m_maxPropDelay // beacons
          + m_maxPropDelay + m_maxPropDelay
          + m_maxPropDelay + m_maxPropDelay
          + Seconds (DataRate(m_modes[m_currentChannel].GetDataRateBps ()).CalculateTxTime (100));

  m_waitCtsEvent = Simulator::Schedule(totalDelay, &UanMacCumac::CancelWaitCts, this);
}

void
UanMacCumac::CancelWaitCts (void)
{
  NS_LOG_DEBUG ("" << Simulator::Now ().GetSeconds () << " MAC " << m_address << " NO CTS RECEIVED");
  // No cts received... retrying
  m_status = IDLE;

  TryRts ();
}


void
UanMacCumac::RegisterPosition (UanAddress addr, Vector position)
{
  m_positionTable[addr] = position;
  m_positionTable[m_address] = GetPosition ();
}

Time
UanMacCumac::CalculateDelay (UanAddress src, UanAddress dst)
{
  if (m_positionTable.find (src) != m_positionTable.end ()
          && m_positionTable.find (dst) != m_positionTable.end ()) {
    return CalculateDelay (m_positionTable[src], m_positionTable[dst]);
  }

  return m_maxPropDelay;
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

  Time maxRtt = Seconds (550.0 / 1500.0);
  Time tonePulseInterval =  Seconds ((12 + 4 * interval) / 1000.0);

  Simulator::Schedule(maxRtt + tonePulseInterval, &TonePulseTable::Remove, this, id);

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

    if (CalculateDistance (e.m_position, position) <= 550)
      return true;
  }

  return false;
}

}
