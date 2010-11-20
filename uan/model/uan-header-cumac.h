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


#ifndef UAN_HEADER_CUMAC_H
#define UAN_HEADER_CUMAC_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/uan-address.h"
#include "ns3/uan-header-common.h"
#include "ns3/vector.h"

#include <set>

namespace ns3 {

typedef enum { DATA, RTS, CTS, BEACON } UanMacCumacPacketType;

typedef std::set<uint8_t> ChannelList;

/**
 * \class UanHeaderCumacData
 *
 * \brief Extra data header information
 *
 * Adds frame number info to the transmitted data packet
 */
class UanHeaderCumacData : public UanHeaderCommon
{
public:
  UanHeaderCumacData ();

  UanHeaderCumacData (uint8_t frameNum);
  virtual ~UanHeaderCumacData ();

  static TypeId GetTypeId (void);

  /**
   * \param frameNum Data frame # of reservation being transmitted
   */
  void SetFrameNo (uint8_t frameNum);

  /**
   * \returns Data frame # of reservation being transmitted
   */
  uint8_t GetFrameNo (void) const;

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;

private:
  uint8_t m_frameNo;
};

/**
 * \class UanHeaderCumacRts
 *
 * \brief RTS header
 *
 * Contains frame #, length, position, channels
 */
class UanHeaderCumacRts : public UanHeaderCommon
{
public:
  UanHeaderCumacRts ();
  /**
   * \param frameNo Reservation frame #
   * \param length # of bytes (including headers) in data
   * \param position of the node
   * \param channelList of available channels
   */
  UanHeaderCumacRts (uint8_t frameNo, uint16_t length, Vector position, ChannelList channels);
  virtual ~UanHeaderCumacRts ();

  static TypeId GetTypeId (void);

  /**
   * \param fno TX frame #
   */
  void SetFrameNo (uint8_t fno);

  /**
   * \param length Total number of data bytes in reservation (including headers)
   * \note Timestamp is serialized with 32 bits in ms precision
   */
  void SetLength (uint16_t length);

  /**
   * \returns Frame #
   */
  uint8_t GetFrameNo (void) const;

  /**
   * \returns Total # of data bytes
   */
  uint16_t GetLength (void) const;

  /**
   * \returns the channel list
   */
  ChannelList GetChannelList (void) const;

  /**
   * \returns the position
   */
  Vector GetPosition (void) const;

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;

private:
  uint8_t m_frameNo;
  uint16_t m_length;
  Vector m_position;
  ChannelList m_channels;
};

/**
 * \class UanHeaderCumacBeacon
 *
 * \brief Cycle broadcast information for
 */


class UanHeaderCumacBeacon : public UanHeaderCommon
{
public:
  /**
   * \brief Create UanHeaderCumacBeacon
   */
  UanHeaderCumacBeacon ();
  /**
   * \brief
   * \param
   */
  UanHeaderCumacBeacon (uint8_t channel, uint8_t signalInterval, Vector dstPosition);
  ~UanHeaderCumacBeacon ();

  static TypeId GetTypeId (void);

  uint8_t GetChannel (void) const;

  uint8_t GetSignalInterval (void) const;

  Vector GetDstPosition (void) const;

  void SetChannel (uint8_t channel);

  void SetSignalInterval (uint8_t signalInterval);

  void SetDstPosition (Vector position);

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;

private:
  uint8_t m_channel;
  uint8_t m_signalInterval;
  Vector m_dstPosition;
};
/**
 * \class UanHeaderCumacCts
 *
 * \brief CTS header
 *
 * Includes RTS RX time, CTS TX time, delay until TX, RTS blocking period,
 * RTS tx period, rate #, and retry rate #
 */

class UanHeaderCumacCts : public UanHeaderCommon
{
public:
  UanHeaderCumacCts ();
  /**
   * \param frameNo Resrvation frame # being cleared
   * \param retryNo Retry # of received RTS packet
   * \param rtsTs RX time of RTS packet at gateway
   * \param delay Delay until transmission
   * \param addr Destination of CTS packet
   * \note Times are serialized, with ms precission, into 32 bit fields.
   */
  UanHeaderCumacCts (uint8_t channel, uint8_t frameNo, uint16_t packetSize, Vector srcPosition, Vector dstPosition);
  virtual ~UanHeaderCumacCts ();

  uint8_t GetFrameNo (void) const;
  uint8_t GetChannel (void) const;
  uint16_t GetPacketSize (void) const;
  Vector GetSrcPosition (void) const;
  Vector GetDstPosition (void) const;

  static TypeId GetTypeId (void);

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;

private:
  uint8_t m_channel;
  uint8_t m_frameNo;
  uint16_t m_packetSize;
  Vector m_srcPosition;
  Vector m_dstPosition;
};

/**
 * \class UanHeaderCumacAck
 * \brief Header used for ACK packets by protocol ns3::UanMacRc
 */
class UanHeaderCumacAck : public Header
{
public:
  UanHeaderCumacAck ();
  virtual ~UanHeaderCumacAck ();

  static TypeId GetTypeId (void);

  /**
   * \param frameNo Frame # of reservation being acknowledged
   */
  void SetFrameNo (uint8_t frameNo);
  /**
   * \param frame Data frame # being nacked
   */
  void AddNackedFrame (uint8_t frame);

  /**
   * \returns Set of nacked frames
   */
  const std::set<uint8_t> &GetNackedFrames (void) const;
  /**
   * \returns Reservation frame # being acknowledged.
   */
  uint8_t GetFrameNo (void) const;
  /**
   * \returns Number of data frames being NACKED
   */
  uint8_t GetNoNacks (void) const;

  // Inherrited methods
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  virtual TypeId GetInstanceTypeId (void) const;

private:
  uint8_t m_frameNo;
  std::set<uint8_t> m_nackedFrames;

};

}

#endif // UAN_HEADER_CUMAC_H
