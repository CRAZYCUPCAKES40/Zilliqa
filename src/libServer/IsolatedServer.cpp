/*
 * Copyright (C) 2019 Zilliqa
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "IsolatedServer.h"
#include "JSONConversion.h"

using namespace jsonrpc;
using namespace std;

IsolatedServer::IsolatedServer(Mediator& mediator,
                               AbstractServerConnector& server)
    : LookupServer(mediator, server),
      jsonrpc::AbstractServer<IsolatedServer>(server,
                                              jsonrpc::JSONRPC_SERVER_V2) {
  AbstractServer<IsolatedServer>::bindAndAddMethod(
      jsonrpc::Procedure("CreateTransaction", jsonrpc::PARAMS_BY_POSITION,
                         jsonrpc::JSON_OBJECT, "param01", jsonrpc::JSON_OBJECT,
                         NULL),
      &IsolatedServer::CreateTransactionI);
}

Json::Value IsolatedServer::CreateTransaction(const Json::Value& _json) {
  try {
    if (!JSONConversion::checkJsonTx(_json)) {
      throw JsonRpcException(RPC_PARSE_ERROR, "Invalid Transaction JSON");
    }

    LOG_GENERAL(INFO, "On the isolated server ");

    Transaction tx = JSONConversion::convertJsontoTx(_json);

    Json::Value ret;

    const Address fromAddr = tx.GetSenderAddr();
    const Account* sender = AccountStore::GetInstance().GetAccount(fromAddr);

    if (!ValidateTxn(tx, fromAddr, sender)) {
      return ret;
    }

    TransactionReceipt txreceipt;
    AccountStore::GetInstance().UpdateAccountsTemp(1000, 3  // Arbitrary values
                                                   ,
                                                   true, tx, txreceipt);

    AccountStore::GetInstance().SerializeDelta();
    AccountStore::GetInstance().CommitTemp();

    return txreceipt.GetJsonValue();

  } catch (const JsonRpcException& je) {
    throw je;
  } catch (exception& e) {
    LOG_GENERAL(INFO,
                "[Error]" << e.what() << " Input: " << _json.toStyledString());
    throw JsonRpcException(RPC_MISC_ERROR, "Unable to Process");
  }

  return Json::Value();
}