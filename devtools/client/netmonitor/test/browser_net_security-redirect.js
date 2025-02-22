/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

/**
 * Test a http -> https redirect shows secure icon only for redirected https
 * request.
 */

add_task(function* () {
  let { tab, monitor } = yield initNetMonitor(CUSTOM_GET_URL);
  let { document, gStore, windowRequire } = monitor.panelWin;
  let Actions = windowRequire("devtools/client/netmonitor/actions/index");
  let {
    getDisplayedRequests,
    getSortedRequests,
  } = windowRequire("devtools/client/netmonitor/selectors/index");

  gStore.dispatch(Actions.batchEnable(false));

  let wait = waitForNetworkEvents(monitor, 2);
  yield ContentTask.spawn(tab.linkedBrowser, HTTPS_REDIRECT_SJS, function* (url) {
    content.wrappedJSObject.performRequests(1, url);
  });
  yield wait;

  is(gStore.getState().requests.requests.size, 2, "There were two requests due to redirect.");

  let initial = getSortedRequests(gStore.getState()).get(0);
  let redirect = getSortedRequests(gStore.getState()).get(1);

  let initialSecurityIcon = document.querySelectorAll(".requests-security-state-icon")[0];
  let redirectSecurityIcon = document.querySelectorAll(".requests-security-state-icon")[1];

  ok(initialSecurityIcon.classList.contains("security-state-insecure"),
     "Initial request was marked insecure.");

  ok(redirectSecurityIcon.classList.contains("security-state-secure"),
     "Redirected request was marked secure.");

  yield teardown(monitor);
});
