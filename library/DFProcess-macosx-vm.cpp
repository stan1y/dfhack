uint32

QString NormalProcess::read_string(const uint &addr) {
	uint32 buffer_addr = read_addr(addr);
	int upper_size = 256;
	QByteArray buf(upper_size, 0);
	read_raw(buffer_addr, upper_size, buf);
	
	QString ret_val(buf);
	CP437Codec *codec = new CP437Codec;
	ret_val = codec->toUnicode(ret_val.toAscii());
	return ret_val;
}

int NormalProcess::write_string(const uint32 &addr, const QString &str) {
	Q_UNUSED(addr);
	Q_UNUSED(str);
	return 0;
}

int NormalProcess::write_int(const uint32 &addr, const int &val) {
	return write_raw(addr, sizeof(int), (void*)&val);
}

int NormalProcess::read_raw(const uint32 &addr,
							int bytes, 
							QByteArray &buffer) {

	kern_return_t result;
		
	vm_size_t readsize = 0;
		
	result = vm_read_overwrite(m_task, addr, bytes,
							   (vm_address_t)buffer.data(),
							   &readsize );
	
	if ( result != KERN_SUCCESS ) {
		return 0;
	}
	
	return readsize;
	
}

int NormalProcess::write_raw(const uint32 &addr, const int &bytes, 
							 void *buffer) {
	kern_return_t result;

	result = vm_write( m_task,	addr,  (vm_offset_t)buffer,	 bytes );
	if ( result != KERN_SUCCESS ) {
		return 0;
	}

	return bytes;
}

bool NormalProcess::find_running_copy(bool connect_anyway) {
	//all fine at the start
	m_is_ok = true;
	
	NSAutoreleasePool *authPool = [[NSAutoreleasePool alloc] init];	 
	NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
	NSArray *launchedApps = [workspace launchedApplications];
	unsigned i, len = [launchedApps count];
	
	// compile process array
	for ( i = 0; i < len; i++ ) {
		NSDictionary *application = [launchedApps objectAtIndex:i];
		if ( [[application objectForKey:@"NSApplicationName"] 
											isEqualToString:@"dwarfort.exe" ]) {
			
			m_loc_of_dfexe = QString( [[application objectForKey:
									 @"NSApplicationPath"] UTF8String]);
			m_pid = [[application objectForKey:
											@"NSApplicationProcessIdentifier"] 
											intValue];
		}
	}
	LOGD << "Location of exe" << m_loc_of_dfexe;
	if (m_pid == 0 || ! (task_for_pid( current_task(), m_pid, &m_task ) == KERN_SUCCESS)) {
		QMessageBox::warning(0, tr("Warning"),
							 tr("Unable to locate a running copy of Dwarf "
								"Fortress, are you sure it's running?"));
		LOGW << "can't find running copy";
		m_is_ok = false;
	}
	else {
		m_layout = get_memory_layout(hexify(calculate_checksum()).toLower(), !connect_anyway);
		map_virtual_memory();
	}
	
	[authPool release];
	return m_is_ok;
}