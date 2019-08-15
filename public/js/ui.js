var synthBox = null;
var pointerDebugging = false;

function testChange(e) {
	console.log("test");
}

function createKnob( id, label, width, x, y, min, max, currentValue, color, onChange, units, log ) {
	var container = document.createElement( "div" );
	container.className = "knobContainer";
	container.style.left = "" + x + "px";
	container.style.top = "" + y + "px";
	var knob = document.createElement( "webaudio-knob" );
	knob.id = id;
	knob.setAttribute( "value", "" + currentValue );
	knob.setAttribute( "src", "img/LittlePhatty.png" );
	knob.setAttribute( "min", ""+min );
	knob.setAttribute( "max", ""+max );
	if (log)
		knob.setAttribute( "log", true );
	else
		knob.setAttribute( "step", (max-min)/100 );
	if (units)
		knob.setAttribute("units", units);
	knob.setAttribute( "diameter", "64" );
	knob.setAttribute( "sprites", "100" );
	knob.setAttribute( "tooltip", label );
	knob.ready();
	knob.onchange = onChange;
//	knob.setValue( currentValue );

	container.appendChild( knob );

	var labelText = document.createElement( "div" );
	labelText.className = "knobLabel";
	labelText.style.top = "" + (width* 0.85) + "px";
	labelText.style.width = "" + width + "px";
	labelText.appendChild( document.createTextNode( label ) );

	container.appendChild( labelText );

//	$( knob ).knob({ 'change' : onChange });

	return container;
}

function createDropdown( id, label, x, y, values, selectedIndex, onChange ) {
	var container = document.createElement( "div" );
	container.className = "dropdownContainer";
	container.style.left = "" + x + "px";
	container.style.top = "" + y + "px";

	var labelText = document.createElement( "div" );
	labelText.className = "dropdownLabel";
	labelText.appendChild( document.createTextNode( label ) );
	container.appendChild( labelText );

	var select = document.createElement( "select" );
	select.className = "dropdownSelect";
	select.id = id;
	for (var i=0; i<values.length; i++) {
		var opt = document.createElement("option");
		opt.appendChild(document.createTextNode(values[i]));
		select.appendChild(opt);
	}
	select.selectedIndex = selectedIndex;
	select.onchange = onChange;
	container.appendChild( select );

	return container;
}

function createSection( label, x, y, width, height ) {
	var container = document.createElement( "fieldset" );
	container.className = "section";
	container.style.left = "" + x + "px";
	container.style.top = "" + y + "px";
	container.style.width = "" + width + "px";
	container.style.height = "" + height + "px";

	var labelText = document.createElement( "legend" );
	labelText.className = "sectionLabel";
	labelText.appendChild( document.createTextNode( label ) );

	container.appendChild( labelText );
	return container;
}

function setupSynthUI() {
	synthBox = document.getElementById("synthbox");

	var midi = createSection( "MIDI", 501, 254, 355, 98 );	
	midi.appendChild( createDropdown( "midiIn", "MIDI Input", 280, 15, ["-no MIDI-"], 0, selectMIDIIn ) );
	midi.appendChild( createDropdown( "midiOut", "MIDI Output", 280, 15, ["-no MIDI-"], 0, selectMIDIOut ) );
  synthBox.appendChild( midi );
} 
