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

  void RegisterTransmission (Time start, uint8_t channelNo, Vector srcPosition, Time txDelay);

  bool CanTransmit (Time time, uint8_t channelNo, Vector dstPosition);

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
    Entry (uint8_t channel, Vector srcPosition, Time start, Time txDelay)
    : m_channel (channel), m_srcPosition (srcPosition), m_start(start), m_txDelay (txDelay)
    {
    }

    inline Time CalculateFinishTime (Vector position) {
      double distance = CalculateDistance (m_srcPosition, position);
      return m_start + m_txDelay + Seconds (distance / 1500.0);
    }

    inline Time CalculateFinishTime (void) {
      Vector3D v(m_srcPosition.x, m_srcPosition.y, m_srcPosition.z + 550);
      return CalculateFinishTime (v);
    }

    inline bool IsExpired (Time now) {
      return CalculateFinishTime () < now;
    }

    inline uint8_t GetChannel (void) {
      return m_channel;
    }

    inline Vector GetSrcPosition (void) {
      return m_srcPosition;
    }

    inline Time GetStartTime (void) {
      return m_start;
    }

  private:
    uint8_t m_channel;
    Vector m_srcPosition;
    Time m_start;
    Time m_txDelay;

  };

  typedef std::list<Entry> EntryList;

  EntryList m_transmissions;

protected:
  virtual void DoDispose ();
};

}

#endif // UAN_MAC_CUMAC_CHANNEL_MANAGER_H
