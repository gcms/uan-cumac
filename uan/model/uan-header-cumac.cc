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


#include "uan-header-cumac.h"

#include <set>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (UanHeaderCumacData);
NS_OBJECT_ENSURE_REGISTERED (UanHeaderCumacRts);
NS_OBJECT_ENSURE_REGISTERED (UanHeaderCumacBeacon);
NS_OBJECT_ENSURE_REGISTERED (UanHeaderCumacCts);
NS_OBJECT_ENSURE_REGISTERED (UanHeaderCumacAck);

UanHeaderCumacData::UanHeaderCumacData ()
  : UanHeaderCommon (),
    m_frameNo (0)
{
}

UanHeaderCumacData::UanHeaderCumacData (uint8_t frameNo)
  : UanHeaderCommon (),
    m_frameNo (frameNo)
{

}

UanHeaderCumacData::~UanHeaderCumacData ()
{
}

TypeId
UanHeaderCumacData::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::UanHeaderCumacData")
    .SetParent<Header> ()
    .AddConstructor<UanHeaderCumacData> ()
  ;
  return tid;
}

void
UanHeaderCumacData::SetFrameNo (uint8_t no)
{
  m_frameNo = no;
}

uint8_t
UanHeaderCumacData::GetFrameNo (void) const
{
  return m_frameNo;
}

uint32_t
UanHeaderCumacData::GetSerializedSize (void) const
{
  return UanHeaderCommon::GetSerializedSize () + 1;
}

void
UanHeaderCumacData::Serialize (Buffer::Iterator start) const
{
  UanHeaderCommon::Serialize (start);
  start.Next (UanHeaderCommon::GetSerializedSize ());

  start.WriteU8 (m_frameNo);
}
uint32_t
UanHeaderCumacData::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator rbuf = start;
  rbuf.Next (UanHeaderCommon::Deserialize (start));

  m_frameNo = rbuf.ReadU8 ();

  return rbuf.GetDistanceFrom (start);
}

void
UanHeaderCumacData::Print (std::ostream &os) const
{
  UanHeaderCommon::Print (os);
  os << "Frame No=" << (uint32_t) m_frameNo;
}

TypeId
UanHeaderCumacData::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


UanHeaderCumacRts::UanHeaderCumacRts ()
  : UanHeaderCommon (),
    m_frameNo (0),
    m_length (0)
{

}

UanHeaderCumacRts::UanHeaderCumacRts (uint8_t frameNo, uint16_t length, Vector position, ChannelList channels)
  : UanHeaderCommon (),
    m_frameNo (frameNo),
    m_length (length),
    m_position (position),
    m_channels (channels)
{

}

UanHeaderCumacRts::~UanHeaderCumacRts ()
{

}

TypeId
UanHeaderCumacRts::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::UanHeaderCumacRts")
    .SetParent<Header> ()
    .AddConstructor<UanHeaderCumacRts> ()
  ;
  return tid;

}

void
UanHeaderCumacRts::SetFrameNo (uint8_t no)
{
  m_frameNo = no;
}

void
UanHeaderCumacRts::SetLength (uint16_t length)
{
  m_length = length;
}

uint16_t
UanHeaderCumacRts::GetLength () const
{
  return m_length;
}

ChannelList
UanHeaderCumacRts::GetChannelList (void) const
{
  return m_channels;
}

uint8_t
UanHeaderCumacRts::GetFrameNo (void) const
{
  return m_frameNo;
}

Vector
UanHeaderCumacRts::GetPosition (void) const
{
  return m_position;
}

uint32_t
UanHeaderCumacRts::GetSerializedSize (void) const
{
  return UanHeaderCommon::GetSerializedSize () + 1 + 2 + (3 * 2) + 8;
}

void
UanHeaderCumacRts::Serialize (Buffer::Iterator start) const
{
  UanHeaderCommon::Serialize (start);
  start.Next (UanHeaderCommon::GetSerializedSize ());

  start.WriteU8 (m_frameNo);
  start.WriteU16 (m_length);
  start.WriteU16 (m_position.x);
  start.WriteU16 (m_position.y);
  start.WriteU16 (m_position.z);

  ChannelList::iterator it = m_channels.begin ();
  for (int i = 0; i < 8; i++, it++) {
    uint8_t channel = it != m_channels.end () ? *it : 0;
    start.WriteU8 (channel);
  }
}

uint32_t
UanHeaderCumacRts::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator rbuf = start;
  rbuf.Next (UanHeaderCommon::Deserialize (start));

  m_frameNo = rbuf.ReadU8 ();
  m_length = rbuf.ReadU16 ();
  m_position.x = rbuf.ReadU16 ();
  m_position.y = rbuf.ReadU16 ();
  m_position.z = rbuf.ReadU16 ();

  for (int i = 0; i < 8; i++) {
    uint8_t channel = rbuf.ReadU8 ();

    if (channel != 0)
      m_channels.insert (channel);
  }

  return rbuf.GetDistanceFrom (start);
}

void
UanHeaderCumacRts::Print (std::ostream &os) const
{
  UanHeaderCommon::Print (os);
  os << "Frame #=" << (uint32_t) m_frameNo << "Length=" << m_length;
}

TypeId
UanHeaderCumacRts::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}




UanHeaderCumacBeacon::UanHeaderCumacBeacon ()
  : UanHeaderCommon ()
{

}

UanHeaderCumacBeacon::UanHeaderCumacBeacon (uint8_t channel, uint8_t signalInterval, Vector dstPosition)
  : UanHeaderCommon (),
    m_channel (channel),
    m_signalInterval (signalInterval),
    m_dstPosition (dstPosition)
{

}

UanHeaderCumacBeacon::~UanHeaderCumacBeacon ()
{

}

TypeId
UanHeaderCumacBeacon::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::UanHeaderCumacBeacon")
    .SetParent<Header> ()
    .AddConstructor<UanHeaderCumacBeacon> ()
  ;
  return tid;

}

uint8_t
UanHeaderCumacBeacon::GetChannel (void) const
{
  return m_channel;
}

uint8_t
UanHeaderCumacBeacon::GetSignalInterval (void) const
{
  return m_signalInterval;
}

Vector
UanHeaderCumacBeacon::GetDstPosition (void) const
{
  return m_dstPosition;
}


uint32_t
UanHeaderCumacBeacon::GetSerializedSize (void) const
{
  return UanHeaderCommon::GetSerializedSize () + 1 + 1 + 3 * 2;
}

void
UanHeaderCumacBeacon::SetChannel (uint8_t channel)
{
  m_channel = channel;
}

void
UanHeaderCumacBeacon::SetSignalInterval (uint8_t signalInterval)
{
  m_signalInterval = signalInterval;
}

void
UanHeaderCumacBeacon::SetDstPosition (Vector position)
{
  m_dstPosition = position;
}

void
UanHeaderCumacBeacon::Serialize (Buffer::Iterator start) const
{
  UanHeaderCommon::Serialize (start);
  start.Next (UanHeaderCommon::GetSerializedSize ());

  start.WriteU8 (m_channel);
  start.WriteU8 (m_signalInterval);

  start.WriteU16 (m_dstPosition.x);
  start.WriteU16 (m_dstPosition.y);
  start.WriteU16 (m_dstPosition.z);
}

uint32_t
UanHeaderCumacBeacon::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator rbuf = start;
  rbuf.Next (UanHeaderCommon::Deserialize (start));

  m_channel = rbuf.ReadU8 ();
  m_signalInterval = rbuf.ReadU8 ();

  m_dstPosition.x = rbuf.ReadU16 ();
  m_dstPosition.y = rbuf.ReadU16 ();
  m_dstPosition.z = rbuf.ReadU16 ();

  return rbuf.GetDistanceFrom (start);
}

void
UanHeaderCumacBeacon::Print (std::ostream &os) const
{
  UanHeaderCommon::Print (os);
//  os << "CTS Global (Rate #=" << m_rateNum << ", Retry Rate=" << m_retryRate << ", TX Time=" << m_timeStampTx.GetSeconds () << ", Win Time=" << m_winTime.GetSeconds () << ")";
}

TypeId
UanHeaderCumacBeacon::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

UanHeaderCumacCts::UanHeaderCumacCts ()
  : UanHeaderCommon ()
{

}

UanHeaderCumacCts::UanHeaderCumacCts (uint8_t channel, uint8_t frameNo, uint16_t packetSize, Vector srcPosition, Vector dstPosition)
  : UanHeaderCommon (),
    m_channel (channel),
    m_frameNo (frameNo),
    m_packetSize (packetSize),
    m_srcPosition (srcPosition),
    m_dstPosition (dstPosition)
{

}

UanHeaderCumacCts::~UanHeaderCumacCts ()
{

}

TypeId
UanHeaderCumacCts::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::UanHeaderCumacCts")
    .SetParent<Header> ()
    .AddConstructor<UanHeaderCumacCts> ()
  ;
  return tid;

}

uint8_t
UanHeaderCumacCts::GetFrameNo (void) const
{
  return m_frameNo;
}

uint8_t
UanHeaderCumacCts::GetChannel (void) const
{
  return m_channel;
}

uint16_t
UanHeaderCumacCts::GetPacketSize (void) const
{
  return m_packetSize;
}

Vector
UanHeaderCumacCts::GetSrcPosition (void) const
{
  return m_srcPosition;
}

Vector
UanHeaderCumacCts::GetDstPosition (void) const
{
  return m_dstPosition;
}

uint32_t
UanHeaderCumacCts::GetSerializedSize (void) const
{
  return UanHeaderCommon::GetSerializedSize () + 1 + 1 + 2 + 3 * 2 + 3 * 2;
}


void
UanHeaderCumacCts::Serialize (Buffer::Iterator start) const
{
  UanHeaderCommon::Serialize (start);
  start.Next (UanHeaderCommon::GetSerializedSize ());

  start.WriteU8 (m_channel);
  start.WriteU8 (m_frameNo);
  start.WriteU16 (m_packetSize);

  start.WriteU16 (m_srcPosition.x);
  start.WriteU16 (m_srcPosition.y);
  start.WriteU16 (m_srcPosition.z);

  start.WriteU16 (m_dstPosition.x);
  start.WriteU16 (m_dstPosition.y);
  start.WriteU16 (m_dstPosition.z);
}

uint32_t
UanHeaderCumacCts::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator rbuf = start;
  rbuf.Next (UanHeaderCommon::Deserialize (start));

  m_channel = rbuf.ReadU8 ();
  m_frameNo = rbuf.ReadU8 ();
  m_packetSize = rbuf.ReadU16 ();

  m_srcPosition.x = rbuf.ReadU16 ();
  m_srcPosition.y = rbuf.ReadU16 ();
  m_srcPosition.z = rbuf.ReadU16 ();

  m_dstPosition.x = rbuf.ReadU16 ();
  m_dstPosition.y = rbuf.ReadU16 ();
  m_dstPosition.z = rbuf.ReadU16 ();

  return rbuf.GetDistanceFrom (start);
}

void
UanHeaderCumacCts::Print (std::ostream &os) const
{
  UanHeaderCommon::Print (os);
//  os << "CTS (Addr=" << m_address << " Frame #=" << (uint32_t) m_frameNo << " Retry #=" << (uint32_t) m_retryNo << " RTS Rx Timestamp=" << m_timeStampRts.GetSeconds () << " Delay until TX=" << m_delay.GetSeconds () << ")";
}

TypeId
UanHeaderCumacCts::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

UanHeaderCumacAck::UanHeaderCumacAck ()
  : m_frameNo (0)
{
}

UanHeaderCumacAck::~UanHeaderCumacAck ()
{
  m_nackedFrames.clear ();
}

TypeId
UanHeaderCumacAck::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::UanHeaderCumacAck")
    .SetParent<Header> ()
    .AddConstructor<UanHeaderCumacAck> ()
  ;
  return tid;
}

void
UanHeaderCumacAck::SetFrameNo (uint8_t noFrames)
{
  m_frameNo = noFrames;
}

void
UanHeaderCumacAck::AddNackedFrame (uint8_t frame)
{
  m_nackedFrames.insert (frame);
}

const std::set<uint8_t> &
UanHeaderCumacAck::GetNackedFrames (void) const
{
  return m_nackedFrames;
}

uint8_t
UanHeaderCumacAck::GetFrameNo (void) const
{
  return m_frameNo;
}

uint8_t
UanHeaderCumacAck::GetNoNacks (void) const
{
  return m_nackedFrames.size ();
}

uint32_t
UanHeaderCumacAck::GetSerializedSize (void) const
{
  return 1 + 1 + GetNoNacks ();
}

void
UanHeaderCumacAck::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_frameNo);
  start.WriteU8 (GetNoNacks ());
  std::set<uint8_t>::iterator it = m_nackedFrames.begin ();
  for (; it != m_nackedFrames.end (); it++)
    {
      start.WriteU8 (*it);
    }
}

uint32_t
UanHeaderCumacAck::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator rbuf = start;
  m_frameNo = rbuf.ReadU8 ();
  uint8_t noAcks = rbuf.ReadU8 ();
  m_nackedFrames.clear ();
  for (uint32_t i = 0; i < noAcks; i++)
    {
      m_nackedFrames.insert (rbuf.ReadU8 ());
    }
  return rbuf.GetDistanceFrom (start);
}

void
UanHeaderCumacAck::Print (std::ostream &os) const
{
  os << "# Frames=" << (uint32_t) m_frameNo << " # nacked=" << (uint32_t) GetNoNacks () << " Nacked: ";
  if (GetNoNacks () > 0)
    {
      std::set<uint8_t>::iterator it = m_nackedFrames.begin ();
      os << (uint32_t) *it;
      it++;
      for (; it != m_nackedFrames.end (); it++)
        {
          os << ", " << (uint32_t) *it;
        }
    }
}

TypeId
UanHeaderCumacAck::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

} // namespace ns3
