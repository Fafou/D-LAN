/**
  * D-LAN - A decentralized LAN file sharing software.
  * Copyright (C) 2010-2012 Greg Burri <greg.burri@gmail.com>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
  
#ifndef CHATSYSTEM_CHATMANAGER_H
#define CHATSYSTEM_CHATMANAGER_H

#include <limits>

#include <QSharedPointer>
#include <QLinkedList>
#include <QTimer>

#include <Common/Uncopyable.h>
#include <Common/Network/MessageHeader.h>
#include <Common/Hash.h>

#include <Core/PeerManager/IPeerManager.h>
#include <Core/NetworkListener/INetworkListener.h>

#include <IChatSystem.h>
#include <priv/ChatMessages.h>
#include <priv/Log.h>

namespace CS
{
   class ChatSystem : public IChatSystem, Common::Uncopyable
   {
      Q_OBJECT
   public:
      ChatSystem(QSharedPointer<PM::IPeerManager> peerManager, QSharedPointer<NL::INetworkListener> networkListener);
      ~ChatSystem();

      void send(const QString& message);
      void getLastChatMessages(Protos::Common::ChatMessages& chatMessages, int number = std::numeric_limits<int>::max()) const;

   private slots:
      void received(const Common::Message& message);
      void getLastChatMessages();
      void saveChatMessages();

   private:
      LOG_INIT_H("ChatSystem");

      QSharedPointer<PM::IPeerManager> peerManager;
      QSharedPointer<NL::INetworkListener> networkListener;

      ChatMessages messages;

      QTimer saveChatMessagesTimer;

      QTimer getLastChatMessageTimer;
      MTRand mtrand;
   };
}
#endif
