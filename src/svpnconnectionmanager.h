/*
 * svpn-jingle
 * Copyright 2013, University of Florida
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SJINGLE_SVPNCONNECTIONMANAGER_H_
#define SJINGLE_SVPNCONNECTIONMANAGER_H_
#pragma once

#include <string>
#include <map>
#include <set>

#include "talk/base/sigslot.h"
#include "talk/p2p/base/p2ptransport.h"
#include "talk/p2p/client/basicportallocator.h"
#include "talk/p2p/base/transportdescription.h"
#include "talk/p2p/base/transportchannelimpl.h"
#include "talk/base/opensslidentity.h"
#include "talk/p2p/base/dtlstransportchannel.h"
#include "talk/p2p/base/dtlstransport.h"
#include "talk/base/base64.h"
#include "talk/p2p/base/basicpacketsocketfactory.h"
#include "talk/base/asyncpacketsocket.h"
#include "talk/base/scoped_ref_ptr.h"
#include "talk/base/refcount.h"

#include "talk/socialvpn/svpn-core/lib/threadqueue/threadqueue.h"
#include "talk/socialvpn/svpn-core/src/svpn.h"
#include "talk/socialvpn/svpn-core/src/tap.h"
#include "talk/socialvpn/svpn-core/src/peerlist.h"
#include "talk/socialvpn/svpn-core/src/packetio.h"

#include "xmppnetwork.h"

namespace sjingle {

class INotifier {
 public:
  virtual void Send(const char* msg, int len) = 0;
};

class SvpnConnectionManager : public talk_base::MessageHandler,
                              public sigslot::has_slots<> {

 public:
  SvpnConnectionManager(SocialNetworkSenderInterface* social_sender,
                        talk_base::Thread* signaling_thread,
                        talk_base::Thread* worker_thread,
                        struct threadqueue* send_queue,
                        struct threadqueue* rcv_queue);

  // Accessors
  const std::string fingerprint() const { return fingerprint_; }

  const std::string uid() const { return svpn_id_; }

  const std::string ipv4() const { return svpn_ip4_; }

  const std::string ipv6() const { return svpn_ip6_; }

  const std::string tap_name() const { return tap_name_; }

  talk_base::Thread* worker_thread() const { return worker_thread_; }

  void set_ip(const char* ip) { svpn_ip4_ = ip; }

  void set_notifier(INotifier* notifier) { notifier_ = notifier; }

  // Signal handlers for BasicNetworkManager
  virtual void OnNetworksChanged();

  // Signal handlers for TransportChannelImpl
  virtual void OnRequestSignaling(cricket::Transport* transport);
  virtual void OnRWChangeState(cricket::Transport* transport);
  virtual void OnCandidatesReady(cricket::Transport* transport,
                                const cricket::Candidates& candidates);
  virtual void OnCandidatesAllocationDone(cricket::Transport* transport);
  virtual void OnReadPacket(cricket::TransportChannel* channel, 
                            const char* data, size_t len, int flags);

  // Inherited from MessageHandler
  virtual void OnMessage(talk_base::Message* msg);

  // Signal handler for SocialNetworkSenderInterface
  virtual void HandlePeer(const std::string& uid, const std::string& data);

  // Signal handler for PacketSenderInterface
  virtual void HandlePacket(talk_base::AsyncPacketSocket* socket,
      const char* data, size_t len, const talk_base::SocketAddress& addr);

  // Other public functions
  virtual void Setup(
      const std::string& uid, const std::string& ip4, int ip4_mask,
      const std::string& ip6, int ip6_mask);

  virtual bool CreateTransport(
      const std::string& uid, const std::string& fingerprint, int nid,
      const std::string& stun_server, const std::string& turn_server,
      const bool sec_enabled);

  virtual bool AddIP(const std::string& uid_key, const std::string& ip4,
                     const std::string& ip6);

  virtual bool DestroyTransport(const std::string& uid);

  virtual std::string GetState();

  // Signal fired when packet inserted in recv_queue
  static void HandleQueueSignal(struct threadqueue* queue);

  typedef cricket::DtlsTransport<cricket::P2PTransport> DtlsP2PTransport;

  struct PeerState {
    std::string uid;
    std::string fingerprint;
    talk_base::scoped_ptr<cricket::P2PTransport> transport;
    talk_base::scoped_ptr<cricket::BasicPortAllocator> port_allocator;
    talk_base::scoped_ptr<talk_base::SSLFingerprint> remote_fingerprint;
    talk_base::scoped_ptr<cricket::TransportDescription> local_description;
    talk_base::scoped_ptr<cricket::TransportDescription> remote_description;
    cricket::Candidates candidates;
    std::set<std::string> candidate_list;
    uint32 last_ping_time;
    int nid;
  };

  typedef talk_base::scoped_refptr<
      talk_base::RefCountedObject<PeerState> > PeerStatePtr;

 private:
  void SetupTransport(PeerState* peer_state);
  bool CreateConnections(const std::string& uid, 
                         const std::string& candidates_string);
  void HandleQueueSignal_w(struct threadqueue* queue);
  void HandlePing_w();
  void UpdateTime(const char* data, size_t len);

  std::string get_key(const std::string& uid) {
    int idx = uid.find('/') + sizeof(kXmppPrefix);
    if ((idx + kIdSize) <= uid.size()) {
      return uid.substr(idx, kIdSize);
    }
    return uid;
  }

  std::string gen_ip6(const std::string& uid_key) {
    int len = (svpn_ip6_.size() - 7) / 2;  // len should be 16
    if (uid_key.size() < len) return "";
    std::string result = svpn_ip6_.substr(0, len + 3);
    for (int i = 0; i < len; i+=4) {
      result += ":";
      result += uid_key.substr(i, 4);
    }
    return result;
  }

  const std::string content_name_;
  SocialNetworkSenderInterface* social_sender_;
  talk_base::BasicPacketSocketFactory packet_factory_;
  std::map<std::string, PeerStatePtr> uid_map_;
  std::map<cricket::Transport*, std::string> transport_map_;
  std::map<std::string, std::string> ip_map_;
  talk_base::Thread* signaling_thread_;
  talk_base::Thread* worker_thread_;
  talk_base::BasicNetworkManager network_manager_;
  std::string svpn_id_;
  talk_base::scoped_ptr<talk_base::OpenSSLIdentity> identity_;
  talk_base::scoped_ptr<talk_base::SSLFingerprint> local_fingerprint_;
  std::string fingerprint_;
  struct threadqueue* send_queue_;
  struct threadqueue* rcv_queue_;
  const uint64 tiebreaker_;
  uint32 check_counter_;
  std::string svpn_ip4_;
  std::string svpn_ip6_;
  std::string tap_name_;
  INotifier* notifier_;
};

}  // namespace sjingle

#endif  // SJINGLE_SVPNCONNECTIONMANAGER_H_

