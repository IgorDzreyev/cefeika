// -*- C++ -*-
// Copyright (C) Dmitry Igrishin
// For conditions of distribution and use, see files LICENSE.txt or pgfe.hpp

#ifndef DMITIGR_PGFE_CONNECTION_HPP
#define DMITIGR_PGFE_CONNECTION_HPP

#include "dmitigr/pgfe/dll.hpp"
#include "dmitigr/pgfe/prepared_statement_dfn.hpp"
#include "dmitigr/pgfe/types_fwd.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace dmitigr::pgfe {

/**
 * @ingroup main
 *
 * @brief A connection to a PostgreSQL server.
 */
class Connection {
public:
  /**
   * @brief The destructor.
   */
  virtual ~Connection() = default;

  /// @name Constructors
  /// @{

  /**
   * @returns A new instance of this class.
   *
   * @param options - the connection options. The value of `nullptr` means
   * default connection options.
   */
  static DMITIGR_PGFE_API std::unique_ptr<Connection> make(const Connection_options* options = nullptr);

  /**
   * @returns The copy of this instance.
   *
   * @remarks The communication status of a copy will be Communication_status::disconnected.
   */
  virtual std::unique_ptr<Connection> to_connection() const = 0;

  /// @}

  // -----------------------------------------------------------------------------

  /// @name General observers
  /// @{

  /**
   * @returns The communication status.
   */
  virtual Communication_status communication_status() const = 0;

  /**
   * @returns `true` if the communication status is Communication_status::connected.
   */
  virtual bool is_connected() const = 0;

  /**
   * @returns `true` if the connection secured by SSL, or `false` otherwise.
   */
  virtual bool is_ssl_secured() const = 0;

  /**
   * @returns The last reported server transaction block status, or
   * `std::nullopt` if unavailable.
   */
  virtual std::optional<Transaction_block_status> transaction_block_status() const = 0;

  /**
   * @returns `true` if transaction block status is Transaction_block_status::uncommitted, or
   * `false` otherwise.
   */
  virtual bool is_transaction_block_uncommitted() const = 0;

  /**
   * @returns The last registered time point when is_connected() started to return `true`, or
   * `std::nullopt` if the session wasn't started.
   */
  virtual std::optional<std::chrono::system_clock::time_point> session_start_time() const noexcept = 0;

  /**
   * @returns The connection options of this instance.
   */
  virtual const Connection_options* options() const noexcept = 0;

  /**
   * @returns The last reported identifier of the server process, or
   * `std::nullopt` if unavailable.
   *
   * @see Notification::server_pid().
   */
  virtual std::optional<std::int_fast32_t> server_pid() const = 0;

  ///@}

  // ---------------------------------------------------------------------------

  /// @name Communication
  /// @{

  /**
   * @brief Establishing the connection to a PostgreSQL server without blocking
   * on I/O.
   *
   * This function should be called repeatedly. Until communication_status()
   * becomes Communication_status::connected or Communication_status::failure loop
   * thus: if communication_status() returned Communication_status::establishment_reading,
   * wait until the socket is ready to read, then call connect_async() again; if
   * communication_status() returned Communication_status::establishment_writing, wait
   * until the socket ready to write, then call connect_async() again. To determine
   * the socket readiness use the socket_readiness() function.
   *
   * @par Effects
   * Possible change of the returned value of communication_status().
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks If called when `(communication_status() == Communication_status::failure)`,
   * it will dismiss all unhandled server messages!
   *
   * @see connect(), communication_status(), socket_readiness().
   */
  virtual void connect_async() = 0;

  /**
   * @brief Attempts to connect to a PostgreSQL server.
   *
   * @param timeout - similar to wait_socket_readiness().
   *
   * @par Effects
   * `(communication_status() == Communication_status::failure ||
   *    communication_status() == Communication_status::connected)`.
   *
   * @par Requires
   * `(timeout >= -1)`.
   *
   * @throws Client_exception with code of Client_errc::timed_out if the
   * `(connection_status() == Communication_status::connected)` will not
   * evaluates to `true` within the specified `timeout`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see connect_async().
   */
  virtual void connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) = 0;

  /**
   * @brief Attempts to disconnect from a server.
   *
   * @par Effects
   * `(communication_status() == Communication_status::disconnected)`.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  virtual void disconnect() = 0;

  /**
   * @brief Waits for readiness of the connection socket if it is unready.
   *
   * @returns The bit mask indicating the readiness of the connection socket.
   *
   * @param mask - the bit mask specifying the requested readiness of the
   * connection socket;
   *
   * @param timeout - the maximum amount of time to wait before return. The
   * special value of `-1` denotes *eternity*.
   *
   * @par Requires
   * `((timeout >= -1) &&
   *    (communication_status() != Communication_status::failure) &&
   *    (communication_status() != Communication_status::disconnected))`.
   */
  virtual Socket_readiness wait_socket_readiness(Socket_readiness mask,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) const = 0;

  /**
   * @brief Polls the readiness of the connection socket.
   *
   * @param mask - similar to wait_socket_readiness().
   *
   * @returns wait_socket_readiness(mask, std::chrono::milliseconds{});
   *
   * @see wait_socket_readiness().
   */
  virtual Socket_readiness socket_readiness(Socket_readiness mask) const = 0;

  ///@}

  // ---------------------------------------------------------------------------

  /// @name Messages
  /// @{

  /**
   * @brief Collects and queue the messages of all kinds which was sent by the server.
   *
   * @par Requires
   * `is_connected()`.
   *
   * @par Effects
   * *Possible* `is_server_message_available()`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void collect_server_messages() = 0;

  /**
   * @returns `(is_signal_available() || is_response_available())`
   */
  virtual bool is_server_message_available() const noexcept = 0;

  /// @}

  // -----------------------------------------------------------------------------

  /**
   * @name Signals
   */
  /// @{

  /**
   * @returns `(notice() || notification())`
   */
  virtual bool is_signal_available() const noexcept = 0;

  /**
   * @returns The pointer to the instance of type Notice if available, or
   * `nullptr` otherwise.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   *
   * @see pop_notice().
   */
  virtual const Notice* notice() const noexcept = 0;

  /**
   * @returns The released instance of type Notice if available, or
   * `nullptr` otherwise.
   *
   * @par Effects
   * notice() returns the pointer to a next instance in the queue, or
   * `nullptr` if the queue is empty.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks A caller should always rely upon assumption that the pointer
   * obtained by notice() becomes invalid after calling this function.
   *
   * @see notice().
   */
  virtual std::unique_ptr<Notice> pop_notice() = 0;

  /**
   * @brief Dismissing an available Notice.
   *
   * @par Effects
   * Same as for pop_notice().
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks It's more efficiently than pop_notice().
   */
  virtual void dismiss_notice() = 0;

  /**
   * @returns The pointer to the instance of type Notification if available, or
   * `nullptr` otherwise.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   *
   * @see pop_notification().
   */
  virtual const Notification* notification() const noexcept = 0;

  /**
   * @returns The released instance of type Notification (with ownership
   * transfer) if available, or `nullptr` otherwise.
   *
   * @par Effects
   * notification() returns the pointer to a next instance in the queue, or
   * `nullptr` if the queue is empty.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks A caller should always rely upon assumption that the pointer
   * obtained by notification() becomes invalid after calling this function.
   *
   * @see notification().
   */
  virtual std::unique_ptr<Notification> pop_notification() = 0;

  /**
   * @brief Dismissing the available instance of type Notification.
   *
   * @par Effects
   * Same as pop_notification().
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks It's more efficiently than pop_notification().
   */
  virtual void dismiss_notification() = 0;

  /**
   * @brief Sets the handler for notices.
   *
   * By default, the notice handler just prints notices to the standard error
   * and never throws.
   *
   * @param handler - the handler to set.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see handle_signals().
   */
  virtual void set_notice_handler(const std::function<void(std::unique_ptr<Notice>&&)>& handler) = 0;

  /**
   * @returns The current notice handler.
   */
  virtual std::function<void(std::unique_ptr<Notice>&&)> notice_handler() const = 0;

  /**
   * @brief Sets the handler for notifications.
   *
   * By default, a notification handler isn't set.
   *
   * @param handler - the handler to set.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see handle_signals().
   */
  virtual void set_notification_handler(const std::function<void(std::unique_ptr<Notification>&&)>& handler) = 0;

  /**
   * @returns The current notification handler.
   */
  virtual std::function<void(std::unique_ptr<Notification>&&)> notification_handler() const = 0;

  /**
   * @brief Call signals handlers.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void handle_signals() = 0;

  ///@}

  // ---------------------------------------------------------------------------

  /// @name Responses
  /// @{

  /**
   * @returns `true` if some kind of the Response is awaited, or `false` otherwise.
   *
   * @see wait_response().
   */
  virtual bool is_awaiting_response() const noexcept = 0;

  /**
   * @brief Waits a some kind of the Response if it is unavailable and awaited.
   *
   * @param timeout - similar to wait_socket_readiness().
   *
   * @par Requires
   * `(timeout >= -1 && is_connected() && is_awaiting_response())`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks All signals retrieved upon waiting the Response will be handled
   * by signals handlers being set.
   *
   * @see is_response_available().
   */
  virtual void wait_response(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) = 0;

  /**
   * @brief Similar to wait_response(), but throws Server_exception
   * if `(error() != nullptr)` after awaiting.
   */
  virtual void wait_response_throw(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) = 0;

  /**
   * @brief Waits for a Completion or an Error.
   *
   * @param timeout - similar to wait_socket_readiness().
   *
   * @par Requires
   * `(timeout >= -1 && is_connected())`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void wait_last_response(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) = 0;

  /**
   * @brief Similar to wait_last_response().
   *
   * @throws Server_exception if `(error() != nullptr)` after awaiting.
   */
  virtual void wait_last_response_throw(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1}) = 0;

  /**
   * @returns `(error() || row() || completion() || prepared_statement())`
   */
  virtual bool is_response_available() const noexcept = 0;

  /**
   * @brief Dismissing the last available Response.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks It's more efficienlty than release_error(), release_row() or
   * release_completion().
   */
  virtual void dismiss_response() = 0;

  /**
   * @returns The pointer to the instance of type Error if available, or
   * `nullptr` otherwise.
   *
   * @remarks This method is semantically similar to release_error() but
   * allows the implementation to avoid extra memory allocation for the
   * retrieved error response.
   *
   * @remarks The object pointed by the returned value is owned by this
   * instance.
   *
   * @see release_error().
   */
  virtual const Error* error() const noexcept = 0;

  /**
   * @returns The released instance of type Error if available, or
   * `nullptr` otherwise.
   *
   * @par Effects
   * `(error() == nullptr)`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks A caller should always rely upon assumption that the pointer
   * obtained by error() becomes invalid after calling this function.
   *
   * @see dismiss_response(), error().
   */
  virtual std::unique_ptr<Error> release_error() = 0;

  /**
   * @returns The pointer to the instance of type Row if available, or
   * `nullptr` otherwise.
   *
   * @remarks This method is semantically similar to release_row() but allows
   * the implementation to avoid extra memory allocation for the each retrieved
   * row.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   *
   * @see release_row().
   */
  virtual const Row* row() const noexcept = 0;

  /**
   * @returns The released instance of type Row if available, or
   * `nullptr` otherwise.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @par Effects
   * `(row() == nullptr)`.
   *
   * @remarks Each Row must be handled or dismissed explicitly. A caller should
   * always rely upon assumption that the pointer obtained by row() becomes
   * invalid after calling this function.
   *
   * @see dismiss_response(), row().
   */
  virtual std::unique_ptr<Row> release_row() = 0;

  /**
   * @returns The pointer to the instance of type Completion if available, or
   * `nullptr` otherwise.
   *
   * @remarks This method is semantically similar to release_completion() but
   * allows the implementation to avoid extra memory allocation for the retrieved
   * completion response.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   *
   * @see release_completion().
   */
  virtual const Completion* completion() const noexcept = 0;

  /**
   * @returns The released instance of type Completion if available, or
   * `nullptr` otherwise.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks There is no necessity to handle Completion explicitly. It will be
   * dismissed automatically when appropriate. A caller should always rely upon
   * assumption that the pointer obtained by completion() becomes invalid after
   * calling this function.
   */
  virtual std::unique_ptr<Completion> release_completion() = 0;

  /**
   * @returns The pointer to the instance of type Prepared_statement if
   * available, or `nullptr` otherwise.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   */
  virtual Prepared_statement* prepared_statement() const = 0;

  /**
   * @returns The prepared statement by its name, or `nullptr` if prepared
   * statement with the given name is unknown by Connection. (This is possible
   * when the statement is prepared by using the SQL command `PREPARE`. Such a
   * statement must be described before using this method.)
   *
   * @param name - the name of the prepared statement.
   *
   * @remarks The object pointed by the returned value is owned by this instance.
   *
   * @see describe_prepared_statement(), describe_prepared_statement_async().
   */
  virtual Prepared_statement* prepared_statement(const std::string& name) const = 0;

  ///@}

  // ---------------------------------------------------------------------------

  /// @name Requests
  /// @{

  /**
   * @returns `true` if the connection is ready for requesting a server in a
   * non-blocking manner, or `false` otherwise.
   *
   * @see is_ready_for_request().
   */
  virtual bool is_ready_for_async_request() const = 0;

  /**
   * @returns `true` if the connection is ready for requesting a server,
   * or `false` otherwise.
   *
   * @see is_awaiting_response().
   */
  virtual bool is_ready_for_request() const = 0;

  /**
   * @brief Submits the SQL query(-es) to a server.
   *
   * @par Awaited responses
   *   - if the query provokes an error: Error;
   *   - if the query does not provokes producing rows: Completion;
   *   - if the query provokes producing rows: the set of Row (if any), and
   *     finally the Completion.
   *
   * @param queries - the string, containing the SQL query(-es). Adjacent
   * queries must be separated by a semicolon.
   *
   * @par Effects
   * `is_awaiting_response()`.
   *
   * @par Requires
   * `is_ready_for_async_request()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks All queries specified in `queries` are executed in a transaction.
   * Each query will provoke producing the separate flow of responses. Therefore,
   * if one of them provokes an Error, then the transaction will be aborted and
   * the queries which were not yet executed will be rejected.
   *
   * @see prepare_statement_async().
   */
  virtual void perform_async(const std::string& queries) = 0;

  /**
   * @brief Similar to perform_async(), but waits for the first Response and
   * throws Server_exception if awaited Response is an Error.
   *
   * @par Requires
   * `is_ready_for_request()`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void perform(const std::string& queries) = 0;

  /**
   * @brief Submits a request to a server to prepare the statement.
   *
   * @par Awaited responses
   * Prepared_statement
   *
   * @param statement - the preparsed SQL string;
   * @param name - the name of the statement to be prepared.
   *
   * @par Effects
   * - `is_awaiting_response()` - just after the successful request submission;
   * - `(prepared_statement(name) != nullptr && prepared_statement(name)->is_preparsed())` - just
   * after the successful response collection.
   *
   * @par Requires
   * `(statement && !statement->has_missing_parameters() && is_ready_for_async_request())`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks It is **strongly** recommended to specify the types of the parameters
   * by using explicit type casts to avoid ambiguities or type mismatch mistakes.
   * For example:
   *   @code{sql}
   *     -- Force to use generate_series(int, int) overload.
   *     SELECT generate_series($1::int, $2::int);
   *   @endcode
   * This forces parameters `$1` and `$2` to be treated as of type `integer`
   * and thus the corresponding overload will be used in this case.
   *
   * @see unprepare_statement_async().
   */
  virtual void prepare_statement_async(const Sql_string* statement, const std::string& name = {}) = 0;

  /**
   * @overload
   *
   * @remarks The statement will be send as-is without any preparsing.
   */
  virtual void prepare_statement_async(const std::string& statement, const std::string& name = {}) = 0;

  /**
   * @returns `(prepare_statement_async(), wait_response_throw(), prepared_statement())`
   *
   * @par Requires
   * `(statement && !statement->has_missing_parameters() && is_ready_for_request())`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks See remarks of prepare_statement_async().
   *
   * @see unprepare_statement().
   */
  virtual Prepared_statement* prepare_statement(const Sql_string* statement, const std::string& name = {}) = 0;

  /**
   * @overload
   *
   * @remarks The statement will be send as-is without any preparsing.
   */
  virtual Prepared_statement* prepare_statement(const std::string& statement, const std::string& name = {}) = 0;

  /**
   * @brief Submits a request to a PostgreSQL server to describe
   * the prepared statement.
   *
   * @par Awaiting responses
   * Prepared_statement
   *
   * @param name - the name of the prepared statement.
   *
   * @par Effects
   * - `is_awaiting_response()` - just after the successful request submission;
   * - `(prepared_statement(name) != nullptr && prepared_statement(name)->is_described())` - just
   * after the successful response collection.
   *
   * @par Requires
   * `is_ready_for_async_request()`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @see describe_prepared_statement().
   */
  virtual void describe_prepared_statement_async(const std::string& name) = 0;

  /**
   * @returns `(describe_prepared_statement_async(), wait_response_throw(), prepared_statement())`
   *
   * @par Requires
   * `is_ready_for_request()`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @see unprepare_statement().
   */
  virtual Prepared_statement* describe_prepared_statement(const std::string& name) = 0;

  /**
   * @brief Submits a request to a PostgreSQL server to close
   * the prepared statement.
   *
   * @par Awaited responses
   * Completion
   *
   * @param name - the name of the prepared statement.
   *
   * @par Effects
   * - `is_awaiting_response()` - just after the successful request submission;
   * - `(prepared_statement(name) == nullptr)` - just after the successful response collection.
   *
   * @par Requires
   * `(is_ready_for_async_request() && !name.empty())`.
   *
   * @par Exception safety guarantee
   * Strong.
   *
   * @remarks It's impossible to unprepare an unnamed prepared statement
   * at the moment.
   *
   * @see unprepare_statement().
   */
  virtual void unprepare_statement_async(const std::string& name) = 0;

  /**
   * @returns `(unprepare_statement_async(const std::string& name), wait_response_throw())`
   *
   * @par Requires
   * `is_ready_for_request()`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void unprepare_statement(const std::string& name) = 0;

  /**
   * @brief Submits the requests to a server to prepare and execute the unnamed
   * statement from the preparsed SQL string, and waits for a response.
   *
   * @par Awaited responses
   * Similar to perform_async().
   *
   * @param statement - the *preparsed* statement to execute;
   * @param parameters - the parameters to bind with the parameterized statement.
   *
   * @par Requires
   * `(statement && !statement->has_missing_parameters() && is_ready_for_request())`.
   *
   * @par Exception safety guarantee
   * Basic.
   *
   * @remarks See remarks of prepare_statement().
   */
  template<typename ... Types>
  void execute(const Sql_string* const statement, Types&& ... parameters)
  {
    auto* const ps = prepare_statement(statement);
    ps->set_parameters(std::forward<Types>(parameters)...);
    ps->execute();
  }

  /**
   * @overload
   *
   * @remarks The statement will be send as-is without any preparsing.
   */
  template<typename ... Types>
  void execute(const std::string& statement, Types&& ... parameters)
  {
    auto* const ps = prepare_statement(statement);
    ps->set_parameters(std::forward<Types>(parameters)...);
    ps->execute();
  }

  /**
   * @brief Sets the default data format of the result for a next prepared
   * statement execution.
   *
   * @param format - the data format to set.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  virtual void set_result_format(Data_format format) = 0;

  /**
   * @returns The default data format of the result for a next prepared
   * statement execution.
   */
  virtual Data_format result_format() const noexcept = 0;

  ///@}

  // ---------------------------------------------------------------------------

  /// @name Utilities
  /// @{

  /**
   * @brief Calls `body(row())` for each awaited row just after it retrieving.
   *
   * @param body - the callback function.
   *
   * @par Requires
   * `(bool(body) == true)`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void for_each(const std::function<void(const Row*)>& body) = 0;

  /**
   * @overload
   *
   * @remarks Calls body(release_row()).
   */
  virtual void for_each(const std::function<void(std::unique_ptr<Row>&&)>& body) = 0;

  /**
   * @brief Waits for the Completion or the Error and calls
   * `body(completion())` if the `body` callback is given.
   *
   * @param body - the callback function.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  virtual void complete(const std::function<void(const Completion*)>& body = {}) = 0;

  /**
   * @overload
   *
   * @remarks Calls body(release_completion()).
   */
  virtual void complete(const std::function<void(std::unique_ptr<Completion>&&)>& body) = 0;

  /**
   * @brief Quotes the given string to be used as a literal in a SQL query.
   *
   * @returns The suitably quoted literal.
   *
   * @param literal - the literal to quote.
   *
   * @par Requires
   * `is_connected()`.
   *
   * @remarks Using of parameterized prepared statement should be considered as
   * the better alternative comparing to including the quoted data into a query.
   *
   * @remarks Note, the result is depends on session properties (such as a
   * character encoding). Therefore using it in queries which are submitted
   * by using connections with other session properties is not correct.
   *
   * @see Prepared_statement.
   */
  virtual std::string to_quoted_literal(const std::string& literal) const = 0;

  /**
   * @brief Quotes the given string to be used as an identifier in a SQL query.
   *
   * @param identifier - the identifier to quote.
   *
   * @returns The suitably quoted identifier.
   *
   * @par Requires
   * `is_connected()`.
   *
   * @remarks Note, the result is depends on session properties (such as a
   * character encoding). Therefore using it in queries which are submitted
   * by using connections with other session properties is not correct.
   *
   * @see Prepared_statement.
   */
  virtual std::string to_quoted_identifier(const std::string& identifier) const = 0;

  /**
   * @brief Encodes the binary data into the textual representation to be used
   * in a SQL query.
   *
   * @param binary_data - the Data of the binary format to escape.
   *
   * @returns The encoded data in the hex format.
   *
   * @par Requires
   * `(is_connected() && binary_data && (binary_data->format() == Data_format::binary))`.
   *
   * @remarks Using of parameterized prepared statement should be considered as
   * the better alternative comparing to including the encoded binary data into
   * a query.
   *
   * @remarks Note, the result is depends on session properties (such as a
   * character encoding). Therefore using it in queries which are submitted
   * by using connections with other session properties is not correct.
   *
   * @see Prepared_statement.
   */
  virtual std::unique_ptr<Data> to_hex_data(const Data* binary_data) const = 0;

  /**
   * @brief Similar to to_hex_data(const Data*).
   *
   * @returns The encoded string in the hex format.
   *
   * @see to_hex_data().
   */
  virtual std::string to_hex_string(const Data* binary_data) const = 0;

  ///@}
private:
  friend detail::iConnection;

  Connection() = default;
};

} // namespace dmitigr::pgfe

#ifdef DMITIGR_PGFE_HEADER_ONLY
#include "dmitigr/pgfe/connection.cpp"
#include "dmitigr/pgfe/prepared_statement_impl.cpp"
#endif

#endif  // DMITIGR_PGFE_CONNECTION_HPP
