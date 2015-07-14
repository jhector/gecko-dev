/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "nsEngineeringMode.h"

#include "nsLiteralString.h"            // for NS_LITERAL_STRING

namespace mozilla {
namespace dom {


NS_IMPL_ISUPPORTS(nsEngineeringMode, nsIEngineeringMode)

nsEngineeringMode::nsEngineeringMode()
{
}

nsEngineeringMode::~nsEngineeringMode()
{
}

NS_IMETHODIMP
nsEngineeringMode::GetValue(const nsAString& aName,
                            nsIEngineeringModeCallback* aCallback)
{
  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  /* Write your corresponding implementation here,
   * then invoke |aCallback->Onsuccess| to return value.
   * Or |aCallback->Onerror| to indicate an error happened.
   *
   * e.g. aCallback->Onsuccess(NS_LITERAL_STRING("bar"));
   * e.g. aCallback->Onerror(-1);
   *
   * The returned value should be a DOMstring,
   * while the returned error is a int32 error code.
   */
  if (aName.IsEmpty()) {
    aCallback->Onerror(-1);
  } else {
    aCallback->Onsuccess(NS_LITERAL_STRING("bar"));
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::SetValue(const nsAString& aName, const nsAString& aValue,
                            nsIEngineeringModeCallback* aCallback)
{
  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  /* Write your corresponding implementation here,
   * then invoke |aCallback->Onsuccess| to indicate successful.
   * Or |aCallback->Onerror| to indicate an error happened.
   *
   * e.g. aCallback->Onsuccess();
   * e.g. aCallback->Onerror(-1);
   *
   * The returned value should be an empty DOMstring (just for passing
   * compiling, won't be caught by the caller),
   * while the returned error is a int32 error code.
   */
  if (aName.IsEmpty() || aValue.IsEmpty()) {
    aCallback->Onerror(-1);
  } else {
    aCallback->Onsuccess(NS_LITERAL_STRING(""));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::SetMessageHandler(nsIEngineeringModeMessageHandler* aHandler)
{
  /* You should keep aHandler somewhere, and invoke it when you have something
   * to notify applications who are listening to EngineeringMode message event.
   */

  return NS_OK;
}

} // dom
} // mozilla
