var voices = new Array();
var audioContext = null;
var isMobile = false;	// we have to disable the convolver on mobile for performance reasons.

var keys = new Array( 256 );
/* old mapping
keys[65] = 60; // = C4 ("middle C")
keys[87] = 61;
keys[83] = 62;
keys[69] = 63;
keys[68] = 64;
keys[70] = 65; // = F4
keys[84] = 66;
keys[71] = 67;
keys[89] = 68;
keys[72] = 69;
keys[85] = 70;
keys[74] = 71;
keys[75] = 72; // = C5
keys[79] = 73;
keys[76] = 74;
keys[80] = 75;
keys[186] = 76;
keys[222] = 77; // = F5
keys[221] = 78;
keys[13] = 79;
keys[220] = 80;
*/

//Lower row: zsxdcvgbhnjm...
keys[16] = 41; // = F2
keys[65] = 42;
keys[90] = 43;
keys[83] = 44;
keys[88] = 45;
keys[68] = 46;
keys[67] = 47;
keys[86] = 48; // = C3
keys[71] = 49;
keys[66] = 50;
keys[72] = 51;
keys[78] = 52;
keys[77] = 53; // = F3
keys[75] = 54;
keys[188] = 55;
keys[76] = 56;
keys[190] = 57;
keys[186] = 58;
keys[191] = 59;

// Upper row: q2w3er5t6y7u...
keys[81] = 60; // = C4 ("middle C")
keys[50] = 61;
keys[87] = 62;
keys[51] = 63;
keys[69] = 64;
keys[82] = 65; // = F4
keys[53] = 66;
keys[84] = 67;
keys[54] = 68;
keys[89] = 69;
keys[55] = 70;
keys[85] = 71;
keys[73] = 72; // = C5
keys[57] = 73;
keys[79] = 74;
keys[48] = 75;
keys[80] = 76;
keys[219] = 77; // = F5
keys[187] = 78;
keys[221] = 79;
keys[220] = 80;

function frequencyFromNoteNumber( note ) {
	return 440 * Math.pow(2,(note-69)/12);
}

function noteOn( note, velocity ) {
	console.log("note on: " + note );
	if (voices[note] == null) {
		// Create a new synth node
		voices[note] = new Voice(note, velocity);
	}
}

function noteOff( note ) {
	if (voices[note] != null) {
		// Shut off the note playing and clear it 
		voices[note].noteOff();
		voices[note] = null;
	}

}

function $(id) {
	return document.getElementById(id);
}

function Voice( note, velocity ) {
	// set up the volume and filter envelopes
	var now = audioContext.currentTime;
}

Voice.prototype.noteOff = function() {
	var now =  audioContext.currentTime;
}

var currentOctave = 3;
var modOscFreqMultiplier = 1;
var moDouble = false;
var moQuadruple = false;

function keyDown( ev ) {
	if ((ev.keyCode==49)||(ev.keyCode==50)) {
		if (ev.keyCode==49)
			moDouble = true;
		else if (ev.keyCode==50)
			moQuadruple = true;
		changeModMultiplier();
	}

	var note = keys[ev.keyCode];
	if (note)
		noteOn( note + 12*(3-currentOctave), 0.75 );
	console.log( "key down: " + ev.keyCode );

	return false;
}

function keyUp( ev ) {
	if ((ev.keyCode==49)||(ev.keyCode==50)) {
		if (ev.keyCode==49)
			moDouble = false;
		else if (ev.keyCode==50)
			moQuadruple = false;
		changeModMultiplier();
	}

	var note = keys[ev.keyCode];
	if (note)
		noteOff( note + 12*(3-currentOctave) );
//	console.log( "key up: " + ev.keyCode );

	return false;
}
var pointers=[];

function touchstart( ev ) {
	for (var i=0; i<ev.targetTouches.length; i++) {
	    var touch = ev.targetTouches[0];
		var element = touch.target;

		var note = parseInt( element.id.substring( 1 ) );
		console.log( "touchstart: id: " + element.id + "identifier: " + touch.identifier + " note:" + note );
		if (!isNaN(note)) {
			noteOn( note + 12*(3-currentOctave), 0.75 );
			var keybox = document.getElementById("keybox")
			pointers[touch.identifier]=note;
		}
	}
	ev.preventDefault();
}

function touchmove( ev ) {
	for (var i=0; i<ev.targetTouches.length; i++) {
	    var touch = ev.targetTouches[0];
		var element = touch.target;

		var note = parseInt( element.id.substring( 1 ) );
		console.log( "touchmove: id: " + element.id + "identifier: " + touch.identifier + " note:" + note );
		if (!isNaN(note) && pointers[touch.identifier] && pointers[touch.identifier]!=note) {
			noteOff(pointers[touch.identifier] + 12*(3-currentOctave));
			noteOn( note + 12*(3-currentOctave), 0.75 );
			var keybox = document.getElementById("keybox")
			pointers[touch.identifier]=note;
		}
	}
	ev.preventDefault();
}

function touchend( ev ) {
	var note = parseInt( ev.target.id.substring( 1 ) );
	console.log( "touchend: id: " + ev.target.id + " note:" + note );
	if (note != NaN)
		noteOff( note + 12*(3-currentOctave) );
	pointers[ev.pointerId]=null;
	var keybox = document.getElementById("keybox")
	ev.preventDefault();
}

function touchcancel( ev ) {
	console.log( "touchcancel" );
	ev.preventDefault();
}

function pointerDown( ev ) {
	var note = parseInt( ev.target.id.substring( 1 ) );
	if (pointerDebugging)
		console.log( "pointer down: id: " + ev.pointerId
			+ " target: " + ev.target.id + " note:" + note );
	if (!isNaN(note)) {
		noteOn( note + 12*(3-currentOctave), 0.75 );
		var keybox = document.getElementById("keybox")
		pointers[ev.pointerId]=note;
	}
	ev.preventDefault();
}

function pointerMove( ev ) {
	var note = parseInt( ev.target.id.substring( 1 ) );
	if (pointerDebugging)
		console.log( "pointer move: id: " + ev.pointerId 
			+ " target: " + ev.target.id + " note:" + note );
	if (!isNaN(note) && pointers[ev.pointerId] && pointers[ev.pointerId]!=note) {
		if (pointers[ev.pointerId])
			noteOff(pointers[ev.pointerId] + 12*(3-currentOctave));
		noteOn( note + 12*(3-currentOctave), 0.75 );
		pointers[ev.pointerId]=note;
	}
	ev.preventDefault();
}

function pointerUp( ev ) {
	var note = parseInt( ev.target.id.substring( 1 ) );
	if (pointerDebugging)
		console.log( "pointer up: id: " + ev.pointerId + " note:" + note );
	if (note != NaN)
		noteOff( note + 12*(3-currentOctave) );
	pointers[ev.pointerId]=null;
	var keybox = document.getElementById("keybox")
	ev.preventDefault();
}


function initAudio() {
	window.AudioContext = window.AudioContext || window.webkitAudioContext;
	try {
    	audioContext = new AudioContext();
  	}
  	catch(e) {
    	alert('The Web Audio API is apparently not supported in this browser.');
  	}

	window.addEventListener('keydown', keyDown, false);
	window.addEventListener('keyup', keyUp, false);
	setupSynthUI();

	isMobile = (navigator.userAgent.indexOf("Android")!=-1)||(navigator.userAgent.indexOf("iPad")!=-1)||(navigator.userAgent.indexOf("iPhone")!=-1);
}

if('serviceWorker' in navigator) {  
  navigator.serviceWorker  
           .register('./service-worker.js')  
           .then(function() { console.log('Service Worker Registered'); });  
}
window.onload=initAudio;
