var websocket;

function initWebSocket() {
  websocket = new WebSocket('ws://' + window.location.hostname + '/ws');
  websocket.onopen = function(event) { console.log('Connected to WebSocket'); };
  websocket.onclose = function(event) { console.log('Disconnected from WebSocket'); };
  websocket.onmessage = function(event) { handleWebSocketMessage(JSON.parse(event.data)); };
}

function startPit(lane) {
  websocket.send('start' + lane);
}

function updateLaneName(lane, name) {
  websocket.send('update' + lane + ':' + name);
}

function handleWebSocketMessage(message) {
  if (message.type === 'update') {
    updateUI(message.data);
  } else if (message.type === 'announce') {
    announcePitting(message.lane, message.pilotName, message.isPitting);
  }
}

function announcePitting(lane, pilotName, isPitting) {
  var text = isPitting ? "Lane " + (lane + 1) + " pilot " + pilotName + "  is pitting" : "Lane " + (lane + 1) + " pilot " + pilotName + " is leaving the pits";
  var utterance = new SpeechSynthesisUtterance(text);
  speechSynthesis.speak(utterance);
}

function updateUI(buttonStates) {
  for (var i = 0; i < buttonStates.length; i++) {
    var lane = document.getElementById('lane' + (i + 1));
    var h2 = lane.getElementsByTagName('h2')[0];
    var button = lane.getElementsByTagName('button')[0];
    var input = lane.getElementsByClassName('pilotName')[0];
    if (buttonStates[i].countdown > 0) {
      button.innerHTML = buttonStates[i].countdown;
      button.disabled = true;
    } else if (buttonStates[i].isPitting) {
      button.innerHTML = 'Leave Pit';
      button.disabled = false;
      button.style.backgroundColor = 'purple';
    } else {
      button.innerHTML = 'Pit';
      button.disabled = false;
      button.style.backgroundColor = ""
    }
    h2.textContent = "Lane " + (i + 1) + (buttonStates[i].pilotName ? ": " + buttonStates[i].pilotName : "");
    input.value = buttonStates[i].pilotName || '';
  }
}

window.onload = function(event) {
  initWebSocket();
}
