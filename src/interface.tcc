#ifndef SIMPLE_RPC_INTERFACE_TCC_
#define SIMPLE_RPC_INTERFACE_TCC_

/*
 * Template library for exporting native C and C++ functions as remote
 * procedure calls.
 *
 * For more information about (variadic) templates:
 * \li http://www.drdobbs.com/cpp/extracting-function-parameter-and-return/240000586
 * \li https://eli.thegreenplace.net/2014/variadic-templates-in-c/
 * \li https://en.cppreference.com/w/cpp/language/parameter_pack
 */
#include "rpcCall.tcc"
#include "signature.tcc"

//! \defgroup interface


/*!
 * Write the signature and documentation of a function.
 *
 * \param io Input / output object.
 * \param f Function pointer.
 * \param doc Function documentation.
 */
template <class I, class F, class D>
void _writeDescription(I& io, F f, D doc) {
  rpcPrint(io, signature(f), ';', doc, _END_OF_STRING);
}


//! Recursion terminator for `_describe()`.
template <class I>
void _describe(I&) {}

/*!
 * Describe a list of functions.
 *
 * \param io Input / output object.
 * \param f Function pointer.
 * \param doc Function documentation.
 * \param args Remaining parameters.
 */
template <class I, class F, class D, class... Args>
void _describe(I& io, F f, D doc, Args... args) {
  /*
   * The first two parameters `f` and `doc` are isolated and passed to
   * `_writeDescription()`. Then a recursive call to process the remaining
   * parameters is made.
   */
  _writeDescription(io, f, doc);
  _describe(io, args...);
}

//! \copydoc _describe(I&, F, D, Args...)
template <class I, class U, class V, class D, class... Args>
void _describe(I& io, Tuple<U, V> t, D doc, Args... args) {
  _writeDescription(io, t.tail.head, doc);
  _describe(io, args...);
}


//! Recursion terminator for `_select()`.
template <class I>
void _select(I&, byte, byte) {}

/*!
 * Select and call a function indexed by `number`.
 *
 * \param io Input / output object.
 * \param number Function index.
 * \param depth Current index.
 * \param f Function pointer.
 * \param - Function documentation.
 * \param args Remaining parameters.
 */
template <class I, class F, class D, class... Args>
void _select(I& io, byte number, byte depth, F f, D, Args... args) {
  /*
   * The parameter `f` and its documentation string are isolated, discarding
   * the latter. If the selected function is encountered (i.e., if `depth`
   * equals `number`), function `f` is called. Otherwise, a new try is made
   * recursively.
   */
  if (depth == number) {
    rpcCall(io, f);
    return;
  }
  _select(io, number, depth + 1, args...);
}


/*! \ingroup interface
 * RPC interface.
 *
 * The `args` parameter pack is a list of pairs (function pointer,
 * documentation). The documentation string can be of type `char const*`, or
 * the PROGMEM `F()` macro can be used to reduce memory footprint.
 *
 * \param io Input / output object.
 * \param args Parameter pairs (function pointer, documentation).
 */
template <class I, class... Args>
void interface(I& io, Args... args) {
  /*
   * One byte is read into `command`, if the value equals `_LIST_REQ`, the list
   * of functions is described. Otherwise, the function indexed by `command` is
   * called.
   */
  if (io.available()) {
    byte command;

    rpcRead(io, &command);

    if (command == _LIST_REQ) {
      rpcPrint(io, _PROTOCOL, _END_OF_STRING);
      rpcPrint(io, _VERSION[0], _VERSION[1], _VERSION[2]);
      rpcPrint(io, hardwareDefs(), _END_OF_STRING);
      _describe(io, args...);
      rpcPrint(io, _END_OF_STRING);
      return;
    }
    _select(io, command, 0, args...);
  }
}

//! Recursion terminator for `interface()`.
template <class... Args>
void interface(Tuple<>, Args...) {}

/*! \ingroup interface
 * Multiple RPC interfaces.
 *
 * Similar to the standard interface , but with support for multiple I/O
 * interfaces, passed as Tuple `t`.
 *
 * \sa interface(I&, Args...)
 *
 * \param t Tuple of input / output objects.
 * \param args Parameter pairs (function pointer, documentation).
 */
template <class... Membs, class... Args>
void interface(Tuple<Membs...> t, Args... args) {
  interface(*t.head, args...);
  interface(t.tail, args...);
}


#endif
