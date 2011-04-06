#include "dispatcher.h"

#include <examples/protobuf/codec/query.pb.h>

#include <iostream>

using std::cout;
using std::endl;

typedef boost::shared_ptr<muduo::Query> QueryPtr;
typedef boost::shared_ptr<muduo::Answer> AnswerPtr;

void onQuery(const muduo::net::TcpConnectionPtr&,
             const QueryPtr& message,
             muduo::Timestamp)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
}

void onAnswer(const muduo::net::TcpConnectionPtr&,
              const AnswerPtr& message,
              muduo::Timestamp)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const muduo::net::TcpConnectionPtr&,
                          const MessagePtr& message,
                          muduo::Timestamp)
{
  cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ProtobufDispatcher dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback<muduo::Query>(onQuery);
  dispatcher.registerMessageCallback<muduo::Answer>(onAnswer);

  muduo::net::TcpConnectionPtr conn;
  muduo::Timestamp t;

  boost::shared_ptr<muduo::Query> query(new muduo::Query);
  boost::shared_ptr<muduo::Answer> answer(new muduo::Answer);
  boost::shared_ptr<muduo::Empty> empty(new muduo::Empty);
  dispatcher.onMessage(conn, query, t);
  dispatcher.onMessage(conn, answer, t);
  dispatcher.onMessage(conn, empty, t);

  google::protobuf::ShutdownProtobufLibrary();
}
