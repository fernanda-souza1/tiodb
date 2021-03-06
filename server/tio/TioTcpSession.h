/*
Tio: The Information Overlord
Copyright 2010 Rodrigo Strauss (http://www.1bit.com.br)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once

#include "Container.h"
#include "Command.h"
#include "../../client/c/tioclient_internals.h"
//#include "TioTcpServer.h"

namespace tio
{
	using std::endl;

	inline TioData Pr1MessageToCppTioData(const PR1_MESSAGE_FIELD_HEADER* field)
	{
		TioData ret;
		char* stringBuffer;

		switch(field->data_type)
		{
		case MESSAGE_FIELD_TYPE_NONE:
			break;
		case TIO_DATA_TYPE_STRING:
			stringBuffer = (char*) (&field[1]);
			ret.Set(stringBuffer, field->data_size);
			break;
		case TIO_DATA_TYPE_INT:
			ret.Set(pr1_message_field_get_int(field));
			break;
		case TIO_DATA_TYPE_DOUBLE:
			ret.Set(pr1_message_field_get_double(field));
			break;
		};

		return ret;
	}

	inline void Pr1MessageAddField(PR1_MESSAGE* message, unsigned short fieldId, int value)
	{
		pr1_message_add_field_int(message, fieldId, value);
	}

	inline void Pr1MessageAddField(PR1_MESSAGE* message, unsigned short fieldId, const string& value)
	{
		pr1_message_add_field_string(message, fieldId, value.c_str());
	}

	inline void Pr1MessageAddField(PR1_MESSAGE* message, unsigned short fieldId, const TioData& tiodata)
	{
		unsigned short fieldType;

		switch(tiodata.GetDataType())
		{
		case TioData::String:
			fieldType = TIO_DATA_TYPE_STRING;
			break;
		case TioData::Int:
			fieldType = TIO_DATA_TYPE_INT;
			break;
		case TioData::Double:
			fieldType = TIO_DATA_TYPE_DOUBLE;
			break;
		default:
			fieldType = TIO_DATA_TYPE_NONE;
			pr1_message_add_field(message, fieldId, fieldType, NULL, 0);
			return;
		}

		pr1_message_add_field(message, fieldId, fieldType, tiodata.AsRaw(), tiodata.GetSize());
	}


inline bool Pr1MessageGetField(const PR1_MESSAGE* message, unsigned int fieldId, TioData* tiodata)
	{
		PR1_MESSAGE_FIELD_HEADER* field = pr1_message_field_find_by_id(message, fieldId);

		if(!field)
			return false;

		*tiodata = Pr1MessageToCppTioData(field);

		return true;
	}

	inline bool Pr1MessageGetField(const PR1_MESSAGE* message, unsigned int fieldId, string* str)
	{
		PR1_MESSAGE_FIELD_HEADER* field = pr1_message_field_find_by_id(message, fieldId);

		if(!field)
			return false;

		if(field->data_type != TIO_DATA_TYPE_STRING)
			return false;

		char* stringBuffer = (char*) (&field[1]);
		*str = string(stringBuffer, stringBuffer + field->data_size);

		return true;
	}

	inline bool Pr1MessageGetField(const PR1_MESSAGE* message, unsigned int fieldId, int* value)
	{
		PR1_MESSAGE_FIELD_HEADER* field = pr1_message_field_find_by_id(message, fieldId);

		if(!field)
			return false;

		if(field->data_type != TIO_DATA_TYPE_INT)
			return false;

		*value = pr1_message_field_get_int(field);

		return true;
	}
	
	inline void Pr1MessageGetHandleKeyValueAndMetadata(const PR1_MESSAGE* message, int* handle, TioData* key, TioData* value, TioData* metadata)
	{
		if(handle)
		{
			*handle = 0;
			Pr1MessageGetField(message, MESSAGE_FIELD_ID_HANDLE, handle);
		}

		if(key)
			Pr1MessageGetField(message, MESSAGE_FIELD_ID_KEY, key);

		if(value)
			Pr1MessageGetField(message, MESSAGE_FIELD_ID_VALUE, value);

		if(metadata)
			Pr1MessageGetField(message, MESSAGE_FIELD_ID_METADATA, metadata);
	}

	inline shared_ptr<PR1_MESSAGE> Pr1CreateMessage()
	{
		return shared_ptr<PR1_MESSAGE>(pr1_message_new(), &pr1_message_delete);
	}
	
	inline shared_ptr<PR1_MESSAGE> Pr1CreateAnswerMessage()
	{
		shared_ptr<PR1_MESSAGE> answer = Pr1CreateMessage();

		pr1_message_add_field_int(answer.get(), MESSAGE_FIELD_ID_COMMAND, TIO_COMMAND_ANSWER);

		return answer;
	}

	inline void Pr1MessageAddFields(shared_ptr<PR1_MESSAGE> message, const TioData* key, const TioData* value, const TioData* metadata)
	{
		if(key && key->GetDataType() != TioData::None)
			Pr1MessageAddField(message.get(), MESSAGE_FIELD_ID_KEY, *key);

		if(value && value->GetDataType() != TioData::None)
			Pr1MessageAddField(message.get(), MESSAGE_FIELD_ID_VALUE, *value);

		if(metadata && metadata->GetDataType() != TioData::None)
			Pr1MessageAddField(message.get(), MESSAGE_FIELD_ID_METADATA, *metadata);
	}

	inline shared_ptr<PR1_MESSAGE> Pr1CreateAnswerMessage(TioData* key, TioData* value, TioData* metadata)
	{
		shared_ptr<PR1_MESSAGE> answer = Pr1CreateAnswerMessage();
					
		Pr1MessageAddFields(answer, key, value, metadata);

		return answer;
	}

	
	using std::shared_ptr;
	using std::weak_ptr;
	using boost::system::error_code;
	using std::stringstream;

	namespace asio = boost::asio;
	using namespace boost::asio::ip;

	class TioTcpServer;

#if 0
	class BinaryProtocolCommand
	{
		typedef vector< shared_ptr<MESSAGE_FIELD_HEADER> > FieldHeaderVector;
		FieldHeaderVector fields_;

		const MESSAGE_FIELD_HEADER* GetFieldById(unsigned int id) const
		{
			//
			// Common, I'm not going to create a functor just for this... I wish I could
			// use C++0x lambda...
			//
			for(FieldHeaderVector::const_iterator i = fields_.begin() ; i != fields_.end() ; ++i)
				if((*i)->field_id == id)
					return (*i).get();
			
			return NULL;
		}

	public:
		explicit BinaryProtocolCommand(const FieldHeaderVector& fields)
			: fields_(fields)
		{}

		int GetFieldAsIntOrZero(unsigned int fieldId) const
		{
			const MESSAGE_FIELD_HEADER* field = GetFieldById(fieldId);

			if(!field || field->data_type != MESSAGE_FIELD_TYPE_INT)
				return 0;

			return message_field_get_int(field);
		}

		const char* GetFieldAsCStringOrZero(unsigned int fieldId) const
		{
			const MESSAGE_FIELD_HEADER* field = GetFieldById(fieldId);

			if(!field || field->data_type != MESSAGE_FIELD_TYPE_STRING)
				return 0;

			return message_field_get_string(field);
		}

		bool GetHandleAndKeyAndValueAndMetadata(unsigned int fieldId, unsigned int* handle, TioData* key, TioData* value, TioData* metadata) const
		{
			if(handle)
				*handle = GetFieldAsIntOrZero(MESSAGE_FIELD_ID_HANDLE);

			if(key)
				GetFieldAsTioData(MESSAGE_FIELD_ID_KEY, key);
				
			if(value)
				GetFieldAsTioData(MESSAGE_FIELD_ID_VALUE, value);

			if(metadata)
				GetFieldAsTioData(MESSAGE_FIELD_ID_METADATA, metadata);
		}

		bool GetFieldAsTioData(unsigned int fieldId, TioData* tioData) const
		{
			const MESSAGE_FIELD_HEADER* field;
			
			tioData->Clear();

			field = GetFieldById(fieldId);

			if(!field)
				return false;

			switch(field->data_type)
			{
			case MESSAGE_FIELD_TYPE_INT:
				tioData->Set(message_field_get_int(field));
				break;
			case MESSAGE_FIELD_TYPE_STRING:
				tioData->Set(message_field_get_string(field), true);
				break;
			case MESSAGE_FIELD_TYPE_DOUBLE:
				tioData->Set(message_field_get_double(field));
				break;

			default:
				return false;
			}

			return true;
		}
	};
#endif

	struct EXTRA_EVENT
	{
		EXTRA_EVENT(int index, const shared_ptr<ITioContainer>& container, const string& eventName, bool readRecord)
		{
			Fill(index, container, eventName, readRecord);
		}

		EXTRA_EVENT(){}

		TioData key, value, metadata;
		string eventName;

		void Fill(int index, 
			const shared_ptr<ITioContainer>& container, 
			const string& eventName,
			bool readRecord)
		{
			ASSERT((size_t)index <= container->GetRecordCount());

			this->eventName = eventName;
			this->key.Set(index);
			
			if(readRecord)
				container->GetRecord(index, &key, &value, &metadata);
		}

	};

	/*
	class Pr1Message : public boost::noncopyable
	{
		PR1_MESSAGE* pr1_message_;

	public:
		Pr1Message(PR1_MESSAGE_HEADER* header)
		{
			pr1_message_ = pr1_message_new_get_buffer_for_receive(header, void** buffer)
		}

		~Pr1Message()
		{
			pr1_message_delete(pr1_message_);
		}

		PR1_MESSAGE_HEADER* GetHeaderPointer()
		{
			if(pr1_message_->stream_buffer->buffer_size < sizeof(PR1_MESSAGE_HEADER))
				pr1_message_->stream_buffer

		}

	};
	*/

	class TioTcpSession : 
		public std::enable_shared_from_this<TioTcpSession>,
		public boost::noncopyable
	{
	private:
		unsigned int id_;

		asio::io_service& io_service_;
		tcp::socket socket_;
		TioTcpServer& server_;

		Command currentCommand_;

		asio::streambuf buf_;

		typedef std::map<unsigned int, pair<shared_ptr<ITioContainer>, string> > HandleMap;

		//               handle             container                  subscription cookie
		typedef std::map<unsigned int, pair<shared_ptr<ITioContainer>, unsigned int> > DiffMap;

		HandleMap handles_;
		
		DiffMap diffs_;

		unsigned int lastHandle_;
		int sentBytes_;
        int pendingSendSize_;
		int maxPendingSendingSize_;

		bool binaryProtocol_;

		static std::ostream& logstream_;

		std::queue<std::function<void (shared_ptr<TioTcpSession>)>> lowPendingBytesThresholdCallbacks_;

        std::queue<std::string> pendingSendData_;
		
		std::list< shared_ptr<PR1_MESSAGE> > pendingBinarySendData_;
		std::vector< asio::const_buffer > beingSendData_;
		shared_ptr<char> binarySendBuffer_;

		struct SUBSCRIPTION_INFO
		{
			SUBSCRIPTION_INFO(unsigned int handle)
			{
				this->handle = handle;
				cookie = 0;
				nextRecord = 0;
				binaryProtocol = false;
				eventFilterStart = 0;
				eventFilterEnd = -1;
			}

			int eventFilterStart;
			int eventFilterEnd;

			unsigned int handle;
			unsigned int cookie;
			unsigned int nextRecord;
			bool binaryProtocol;
			string event_name;
			shared_ptr<ITioContainer> container;
			shared_ptr<ITioResultSet> resultSet;
		};

		//               handle
		typedef std::map<unsigned int, shared_ptr<SUBSCRIPTION_INFO> > SubscriptionMap;
		SubscriptionMap subscriptions_;
		SubscriptionMap pendingSnapshots_;

		typedef std::map<unsigned int, unsigned int > WaitAndPopNextMap;
		WaitAndPopNextMap poppers_;

		vector<string> tokens_;

		bool valid_;

		static int PENDING_SEND_SIZE_BIG_THRESHOLD;
		static int PENDING_SEND_SIZE_SMALL_THRESHOLD;

		void SendString(const string& str);
		void SendStringNow(const string& str);
		
        void UnsubscribeAll();

		void SendPendingSnapshots();

		
				

		void OnBinaryProtocolMessage(PR1_MESSAGE* message, const error_code& err);
		void OnBinaryProtocolMessageHeader(shared_ptr<PR1_MESSAGE_HEADER> header, const error_code& err);
		void ReadBinaryProtocolMessage();


	public:

		void SendResultSetItem(unsigned int queryID, 
			const TioData& key, const TioData& value, const TioData& metadata);

		void SendBinaryErrorAnswer(int errorCode, const string& description);

		void SendPendingBinaryData();

		void OnBinaryMessageSent(const error_code& err, size_t sent);

		void IncreasePendingSendSize(int size)
		{
			pendingSendSize_ += size;

			if(pendingSendSize_ > maxPendingSendingSize_)
				maxPendingSendingSize_ = pendingSendSize_;
		}


		void RegisterLowPendingBytesCallback(std::function<void (shared_ptr<TioTcpSession>)> lowPendingBytesThresholdCallback);

		void DecreasePendingSendSize(int size);

		int pendingSendSize()
		{
			return pendingSendSize_;
		}

		bool IsPendingSendSizeTooBig()
		{
			return pendingSendSize_ > PENDING_SEND_SIZE_BIG_THRESHOLD;
		}

		void SendBinaryMessage(const shared_ptr<PR1_MESSAGE>& message);

		void SendBinaryAnswer(TioData* key, TioData* value, TioData* metadata);

		void SendBinaryAnswer();


		/*
		bool SendBinaryAnswer(const TioData* key, const TioData* value, const TioData* metadata)
		{
			unsigned int bufferSize = sizeof(unsigned int);
			unsigned int fieldCount = 1;

			if(key && key->GetDataType() != TioData::None)
			{
				fieldCount++;
				bufferSize += sizeof(MESSAGE_FIELD_HEADER) + key->GetRawSize();
			}
				

			if(value && value->GetDataType() != TioData::None)
			{
				fieldCount++;
				bufferSize += sizeof(MESSAGE_FIELD_HEADER) + value->GetRawSize();
			}

			if(metadata && metadata->GetDataType() != TioData::None)
			{
				fieldCount++;
				bufferSize += sizeof(MESSAGE_FIELD_HEADER) + metadata->GetRawSize();
			}

			shared_ptr<char> buffer(new char[bufferSize]);

		}
		*/

		TioTcpSession(asio::io_service& io_service, TioTcpServer& server, unsigned int id);
		~TioTcpSession();
		void LoadDispatchMap();

		tcp::socket& GetSocket();
		void OnAccept();
		void ReadCommand();

		unsigned int id();
		bool UsesBinaryProtocol() const;

		void SendResultSet(shared_ptr<ITioResultSet> resultSet, unsigned int queryID);

		void SendResultSetStart(unsigned int queryID);
		void SendResultSetEnd(unsigned int queryID);

		bool IsValid();

		void OnReadCommand(const error_code& err, size_t read);
		void OnWrite(char* buffer, size_t bufferSize, const error_code& err, size_t read);
		void OnReadMessage(const error_code& err);
		bool CheckError(const error_code& err);
		void OnCommandData(size_t dataSize, const error_code& err, size_t read);
		void SendAnswer(stringstream& answer);
		void SendAnswer(const string& answer);

		unsigned int RegisterContainer(const string& containerName, shared_ptr<ITioContainer> container);
		shared_ptr<ITioContainer> GetRegisteredContainer(unsigned int handle, string* containerName = NULL, string* containerType = NULL);
		void CloseContainerHandle(unsigned int handle);

		void OnEvent(shared_ptr<SUBSCRIPTION_INFO> subscriptionInfo, const string& eventName, const TioData& key, const TioData& value, const TioData& metadata);
		void OnPopEvent(unsigned int handle, const string& eventName, const TioData& key, const TioData& value, const TioData& metadata);

		void SendTextEvent(unsigned int handle, const TioData& key, const TioData& value, const TioData& metadata, const string& eventName);
		void SendEvent(shared_ptr<SUBSCRIPTION_INFO> subscriptionInfo, const string& eventName, const TioData& key, const TioData& value, const TioData& metadata);

		void Subscribe(unsigned int handle, const string& start, int filterEnd, bool sendAnswer=true);
		void BinarySubscribe(unsigned int handle, const string& start, bool sendAnswer);
		void Unsubscribe(unsigned int handle);

		const vector<string>& GetTokens();
		void AddToken(const string& token);

		shared_ptr<ITioContainer> GetDiffDestinationContainer(unsigned int handle);
		void SetupDiffContainer(unsigned int handle, shared_ptr<ITioContainer> destinationContainer);
		void StopDiffs();

		
		void SetCommandRunning()
		{
			commandRunning_ = true;
		}
		void UnsetCommandRunning()
		{
			commandRunning_ = false;
		}
		void SendBinaryEvent( int handle, const TioData& key, const TioData& value, const TioData& metadata, const string& eventName );
		void SendBinaryResultSet(shared_ptr<ITioResultSet> resultSet, unsigned int queryID, function<bool(const TioData& key)> filterFunction, unsigned maxRecords);
		void BinaryWaitAndPopNext(unsigned int handle);
		bool ShouldSendEvent(const shared_ptr<SUBSCRIPTION_INFO>& subscriptionInfo, string eventName, const TioData& key, const TioData& value, const TioData& metadata, std::vector<EXTRA_EVENT>* extraEvents);
		bool commandRunning_;


		void InvalidateConnection(const error_code& err);
		
	};		
}
