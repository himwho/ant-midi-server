var voices = new Array();
var audioContext = null;
var isMobile = false;	// we have to disable the convolver on mobile for performance reasons.

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

function initAudio() {
	window.AudioContext = window.AudioContext || window.webkitAudioContext;
	try {
    	audioContext = new AudioContext();
  	}
  	catch(e) {
    	alert('The Web Audio API is apparently not supported in this browser.');
  	}
	setupSynthUI();

	isMobile = (navigator.userAgent.indexOf("Android")!=-1)||(navigator.userAgent.indexOf("iPad")!=-1)||(navigator.userAgent.indexOf("iPhone")!=-1);
}

if('serviceWorker' in navigator) {  
  navigator.serviceWorker  
           .register('./service-worker.js')  
           .then(function() { console.log('Service Worker Registered'); });  
}
window.onload=initAudio;
