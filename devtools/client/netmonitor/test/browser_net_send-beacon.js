/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

/**
 * Tests if beacons are handled correctly.
 */

add_task(function* () {
  let { tab, monitor } = yield initNetMonitor(SEND_BEACON_URL);
 let { gStore, windowRequire } = monitor.panelWin;
  let Actions = windowRequire("devtools/client/netmonitor/actions/index");
  let { getSortedRequests } = windowRequire("devtools/client/netmonitor/selectors/index");

  gStore.dispatch(Actions.batchEnable(false));

  is(gStore.getState().requests.requests.size, 0, "The requests menu should be empty.");

  let wait = waitForNetworkEvents(monitor, 1);
  yield ContentTask.spawn(tab.linkedBrowser, {}, function* () {
    content.wrappedJSObject.performRequest();
  });
  yield wait;

  is(gStore.getState().requests.requests.size, 1, "The beacon should be recorded.");
  let request = getSortedRequests(gStore.getState()).get(0);
  is(request.method, "POST", "The method is correct.");
  ok(request.url.endsWith("beacon_request"), "The URL is correct.");
  is(request.status, "404", "The status is correct.");

  return teardown(monitor);
});
