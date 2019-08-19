var notes = ["E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7", "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8", "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "G#9"]
var midiAccess = null;
var selectMIDIInput = null;
var selectMIDIOutput = null;
var midiIn = null;
var midiOut = null;
var socket = io.connect();
var inputData = new Array(3);

function midiMessageReceived( ev ) {
  var cmd = ev.data[0] >> 4;
  var channel = ev.data[0] & 0xf;
  var noteNumber = ev.data[1];
  var velocity = ev.data[2];

  if (channel == 9)
    return
  if ( cmd==8 || ((cmd==9)&&(velocity==0)) ) { // with MIDI, note on with velocity zero is the same as note off
    // note off
    noteOff( noteNumber );
  } else if (cmd == 9) {
    // note on
    noteOn( noteNumber, velocity/127.0);
    document.getElementById("note").innerHTML = notes[noteNumber-40];
    document.getElementById("velocity").innerHTML = velocity;
  } else if (cmd == 11) {
    controller( noteNumber, velocity/127.0);
  } else if (cmd == 14) {
    // pitch wheel
    pitchWheel( ((velocity * 128.0 + noteNumber)-8192)/8192.0 );
  } else if ( cmd == 10 ) {  // poly aftertouch
    polyPressure(noteNumber,velocity/127)
  } else
  console.log( "" + ev.data[0] + " " + ev.data[1] + " " + ev.data[2])
}

function selectMIDIIn( ev ) {
  if (midiIn)
    midiIn.onmidimessage = null;
  var id = ev.target[ev.target.selectedIndex].value;
  if ((typeof(midiAccess.inputs) == "function"))   //Old Skool MIDI inputs() code
    midiIn = midiAccess.inputs()[ev.target.selectedIndex];
  else
    midiIn = midiAccess.inputs.get(id);
  if (midiIn)
    midiIn.onmidimessage = midiMessageReceived;
}

function selectMIDIOut( ev ) {
  if (midiOut)
    midiOut.onmidimessage = null;
  var id = ev.target[ev.target.selectedIndex].value;
  if ((typeof(midiAccess.outputs) == "function"))   //Old Skool MIDI outputs() code
    midiOut = midiAccess.outputs()[ev.target.selectedIndex];
  else
    midiOut = midiAccess.outputs.get(id);
  if (midiOut)
    socket.on('internal', function(data) {
      inputData = [ data.x, data.y, data.z ];
      console.log("inputData at midi.js inside function: " + inputData);
      midiOut.send( [ inputData[0], inputData[1], inputData[2] ] );
    });
}

function populateMIDIInSelect() {
  // clear the MIDI input select
  selectMIDIInput.options.length = 0;
  if (midiIn && midiIn.state=="disconnected")
    midiIn=null;
  var firstInput = null;

  var inputs=midiAccess.inputs.values();
  for ( var input = inputs.next(); input && !input.done; input = inputs.next()){
    input = input.value;
    if (!firstInput)
      firstInput=input;
    var str=input.name.toString();
    var preferred = !midiIn && ((str.indexOf("MPK") != -1)||(str.indexOf("Keyboard") != -1)||(str.indexOf("keyboard") != -1)||(str.indexOf("KEYBOARD") != -1));

    // if we're rebuilding the list, but we already had this port open, reselect it.
    if (midiIn && midiIn==input)
      preferred = true;

    selectMIDIInput.appendChild(new Option(input.name,input.id,preferred,preferred));
    if (preferred) {
      midiIn = input;
      midiIn.onmidimessage = midiMessageReceived;
    }
  }
  if (!midiIn) {
      midiIn = firstInput;
      if (midiIn)
        midiIn.onmidimessage = midiMessageReceived;
  }
}

function populateMIDIOutSelect() {
  // clear the MIDI input select
  selectMIDIOutput.options.length = 0;
  if (midiOut && midiOut.state=="disconnected")
    midiOut=null;
  var firstOutput = null;

  var outputs=midiAccess.outputs.values();
  for ( var output = outputs.next(); output && !output.done; output = outputs.next()){
    output = output.value;
    if (!firstOutput)
      firstOutput=output;
    var str=output.name.toString();
    var preferred = !midiOut && ((str.indexOf("MPK") != -1)||(str.indexOf("Keyboard") != -1)||(str.indexOf("keyboard") != -1)||(str.indexOf("KEYBOARD") != -1));

    // if we're rebuilding the list, but we already had this port open, reselect it.
    if (midiOut && midiOut==output)
      preferred = true;

    selectMIDIOutput.appendChild(new Option(output.name,output.id,preferred,preferred));
    if (preferred) {
      midiOut = output;
      midiOut.onmidimessage = midiOut.onmidimessage;
    }
  }
  if (!midiOut) {
      midiOut = firstOutput;
      if (midiOut)
        midiOut.onmidimessage = midiOut.onmidimessage;
  }
}

function midiConnectionStateChange( e ) {
  console.log("connection: " + e.port.name + " " + e.port.connection + " " + e.port.state );
  //populateMIDIInSelect();
  populateMIDIOutSelect();
}

function onMIDIStarted( midi ) {
  var preferredIndex = 0;

  midiAccess = midi;

  document.getElementById("synthbox").className = "loaded";
  selectMIDIInput=document.getElementById("midiIn");
  midi.onstatechange = midiConnectionStateChange;
  //populateMIDIInSelect();
  //selectMIDIInput.onchange = selectMIDIIn;

  selectMIDIOutput=document.getElementById("midiOut");
  midi.onstatechange = midiConnectionStateChange;
  populateMIDIOutSelect();
  selectMIDIOutput.onchange = selectMIDIOut;
}

function onMIDISystemError( err ) {
  document.getElementById("synthbox").className = "error";
  console.log( "MIDI not initialized - error encountered:" + err.code );
}

//init: start up MIDI
window.addEventListener('load', function() {   
  if (navigator.requestMIDIAccess)
    navigator.requestMIDIAccess().then( onMIDIStarted, onMIDISystemError );
});