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

#ifndef UAN_MAC_CUMAC_CHANNEL_MANAGER_H
#define UAN_MAC_CUMAC_CHANNEL_MANAGER_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include <list>


#include "uan-address.h"


namespace ns3
{

/**
 * \class UanMacCumacChannelManager
 */
class UanMacCumacChannelManager : public Object
{
public:
  UanMacCumacChannelManager ();

  virtual ~UanMacCumacChannelManager ();
  static TypeId GetTypeId (void);

  void SetMobilityModel (Ptr<MobilityModel> mobility);

  void RegisterTransmission (uint8_t channelNo, Time start, Time finish,
                             Vector srcPosition, Vector dstPosition);

  bool CanTransmit (uint8_t channelNo, Time start, Time finish,
                    Vector srcPosition, Vector dstPosition);

  bool CanTransmitFromSrc (uint8_t channelNo, Time start, Time finish, Vector srcPosition);
  bool CanTransmitToDst (uint8_t channelNo, Time start, Time finish, Vector dstPosition);

  bool IsRegistered (uint8_t channelNo, Vector position);

  void ClearExpired (Time now);


  inline Time CalculateDelay (Vector srcPosition, Vector dstPosition) {
    return Seconds (CalculateDistance (srcPosition, dstPosition) / 1500.0);
  }

  inline Vector GetPosition (void) {
    return m_mobility->GetPosition ();
  }

private:
  Ptr<MobilityModel> m_mobility;

  class Entry {
  public:
    Entry (uint8_t channel, Time start, Time finish, Vector srcPosition, Vector dstPosition)
    : m_channel (channel),
      m_start (start), m_finish (finish),
      m_srcPosition (srcPosition), m_dstPosition (dstPosition)
    {
    }

    inline bool IsExpired (Time now) {
      return m_finish + Seconds (5) < now;
    }

    inline uint8_t GetChannel (void) {
      return m_channel;
    }

    inline Vector GetSrcPosition (void) {
      return m_srcPosition;
    }

    inline Vector GetDstPosition (void) {
      return m_dstPosition;
    }

    inline Time GetStartTime (void) {
      return m_start;
    }

    inline Time GetFinishTime (void) {
      return m_finish;
    }

    inline Time GetStartTimeAtDst (void) {
      return m_start + CalculateDelay (GetSrcPosition (), GetDstPosition ());
    }

    inline Time GetFinishTimeAtDst (void) {
      return m_finish + CalculateDelay (GetSrcPosition (), GetDstPosition ());
    }

    inline Time CalculateDelay (Vector srcPosition, Vector dstPosition) {
      return Seconds (CalculateDistance (srcPosition, dstPosition) / 1500.0);
    }

  private:
    uint8_t m_channel;
    Time m_start;
    Time m_finish;
    Vector m_srcPosition;
    Vector m_dstPosition;
  };

  typedef std::list<Entry> EntryList;

  EntryList m_transmissions;

protected:
  virtual void DoDispose ();
};

}

#endif // UAN_MAC_CUMAC_CHANNEL_MANAGER_H
