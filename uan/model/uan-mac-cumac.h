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

#ifndef UAN_MAC_CUMAC_H
#define UAN_MAC_CUMAC_H

#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uan-phy.h"
#include "uan-mac.h"
#include "uan-header-cumac.h"
#include "uan-mac-cumac-channel-manager.h"
#include "uan-tx-mode.h"



#include <map>

namespace ns3 {

class TonePulseTable : public Object {
public:

  TonePulseTable ();
  virtual ~TonePulseTable();
  static TypeId GetTypeId (void);

  void NotifyBusy (uint8_t channel, uint8_t interval, Vector position);

  bool IsBusy (uint8_t channel, uint8_t interval, Vector position) const;

private:

  void Remove (uint32_t id);

  class Entry {
  public:
    uint8_t m_channel;
    uint8_t m_interval;
    Vector m_position;
  };

  uint32_t m_nextId;
  std::map<uint32_t, Entry> m_entries;
};

}

namespace ns3
{


class UanPhy;
class UanTxMode;

/**
 * \class UanMacCumac
 * \brief ALOHA MAC Protocol
 *
 * The simplest MAC protocol for wireless networks.  Packets enqueued
 * are immediately transmitted.  This MAC attaches a UanHeaderCommon
 * to outgoing packets for address information.  (The type field is not used)
 */
class UanMacCumac : public UanMac,
                    public UanPhyListener
{
public:
  UanMacCumac ();
  virtual ~UanMacCumac ();
  static TypeId GetTypeId (void);


  //Inheritted functions
  Address GetAddress (void);
  virtual void SetAddress (UanAddress addr);
  virtual bool Enqueue (Ptr<Packet> pkt, const Address &dest, uint16_t protocolNumber);
  virtual void SetForwardUpCb (Callback<void, Ptr<Packet>, const UanAddress& > cb);
  virtual void AttachPhy (Ptr<UanPhy> phy);
  virtual Address GetBroadcast (void) const;
  virtual void Clear (void);

//  inline static Ptr<TonePulseTable> GetPulseTable (void) {
//    return Ptr<TonePulseTable> (&m_pulseTable);
//  }
  // PHY listeners
  /// Function called by UanPhy object to notify of packet reception
  virtual void NotifyRxStart (void);
  /// Function called by UanPhy object to notify of packet received successfully
  virtual void NotifyRxEndOk (void);
  /// Function called by UanPhy object to notify of packet received in error
  virtual void NotifyRxEndError (void);
  /// Function called by UanPhy object to notify of channel sensed busy
  virtual void NotifyCcaStart (void);
  /// Function called by UanPhy object to notify of channel no longer sensed busy
  virtual void NotifyCcaEnd (void);
  /// Function called by UanPhy object to notify of outgoing transmission start
  virtual void NotifyTxStart (Time duration);

  void EndTx (void);

private:
  /* protocol settings */
  Time m_maxPropDelay;
  int m_cwMin;
  int m_cwMax;
  Time m_timeSlot;

  typedef enum {
    IDLE,
    SENDING_RTS, WAITING_CTS, SENDING_DATA,
    SENDING_CTS, WAITING_DATA,
    SENDING_BEACON, WAITING_BEACON_RESPONSE
  } Status;

  /* rts sending */
  int m_cw;
  int m_numRetries;
  Time m_timeStartDelay;
  Time m_timeCurrentDelay;
  EventId m_currentTimer;
  bool m_tryingRts;
  bool m_timerRunning;
  bool m_rtsCts;
  EventId m_rtsWaitCtsEvent;

  Status m_status;
  bool m_tx;

  std::map<UanAddress, Vector> m_positionTable;

  void RegisterPosition (UanAddress addr, Vector position);

  Time CalculateDelay (UanAddress src, UanAddress dst);

  UanAddress m_address;
  Ptr<UanPhy> m_phy;

  UanMacCumacChannelManager m_channelManager;

  /* sending packet data */
  bool m_hasPacket;
  uint8_t m_currentFrameNo;
  Ptr<Packet> m_packet;
  uint16_t m_protocolNumber;
  UanAddress m_dstAddress;

  /* channel management */
  uint8_t m_currentChannel;
  UanModesList m_modes;

  /* beacon */
  UanHeaderCumacRts m_rtsReceived;
  ChannelList m_channelsToTry;
  ChannelList::iterator m_currentTryingChannel;
  uint8_t m_signalInterval;
  uint8_t m_beaconCheckTimes;

  /* waiting data */
  EventId m_waitDataEvent;

  /* waiting cts */
  EventId m_waitCtsEvent;


  Callback<void, Ptr<Packet>, const UanAddress& > m_forUpCb;
  bool m_cleared;


  void SetChannel (uint8_t channel);

  Ptr<MobilityModel> GetMobilityModel (void) const;

  Vector GetPosition (void) const;

  Time CalculateDelay (Vector a, Vector b) const;


  /**
   * \brief Receive packet from lower layer (passed to PHY as callback)
   * \param pkt Packet being received
   * \param sinr SINR of received packet
   * \param txMode Mode of received packet
   */
  void RxPacketGood (Ptr<Packet> pkt, double sinr, UanTxMode txMode);

  /**
   * \brief Packet received at lower layer in error
   * \param pkt Packet received in error
   * \param sinr SINR of received packet
   */
  void RxPacketError (Ptr<Packet> pkt, double sinr);

  void StartBeacon (UanHeaderCumacRts &rts);
  void SendBeacon (void);
  void WaitBeacon (void);
  void CheckBeacon (void);

  void StartRts (void);
  void TryRts (void);
  void SendRts (void);

  void StopTimer (void);
  void StartTimer (void);

  void StartCts (void);
  void WaitCts (void);
  void CancelWaitCts (void);

  void WaitData (void);
  void CancelWaitData (void);

protected:
  virtual void DoDispose ();
};

}

#endif // UAN_MAC_CUMAC_H
