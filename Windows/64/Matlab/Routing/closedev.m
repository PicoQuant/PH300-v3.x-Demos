fprintf('\nclosing all PicoHarp devices\n');
if (libisloaded('PHlib'))   
    for(i=0:7); % no harm to close all
        calllib('PHlib', 'PH_CloseDevice', i);
    end;
end;
