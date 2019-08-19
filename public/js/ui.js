var synthBox = null;
var pointerDebugging = false;

function testChange(e) {
	console.log("test");
}

function createDropdown( id, label, x, y, values, selectedIndex, onChange ) {
	var container = document.createElement( "div" );
	container.className = "dropdownContainer";
	container.style.left = "" + x + "px";
	container.style.top = "" + y + "px";
  container.style.width = "100%";

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
	var container = document.createElement( "div" );
	container.className = "section";
	container.style.left = "" + x + "px";
	container.style.top = "" + y + "px";
	container.style.width = "" + width + "%";
	container.style.height = "" + height + "%";

	var labelText = document.createElement( "legend" );
	labelText.className = "sectionLabel";
  labelText.style.marginTop = "10px";
	labelText.appendChild( document.createTextNode( label ) );

	container.appendChild( labelText );
	return container;
}

function setupSynthUI() {
	synthBox = document.getElementById("synthbox");

	var midi = createSection( "MIDI OUTPUT", 0, 0, 100, 100 );	
	//midi.appendChild( createDropdown( "midiIn", "MIDI Input", 280, 15, ["-no MIDI-"], 0, selectMIDIIn ) );
	midi.appendChild( createDropdown( "midiOut", "", 0, 0, ["-no MIDI-"], 0, selectMIDIOut ) );
  synthBox.appendChild( midi );
} 
