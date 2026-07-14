clc;
clear;
close all;

% 1. Ρύθμιση του URI του Jetstream
uri = 'wss://jetstream1.us-east.bsky.network/subscribe?wantedCollections=app.bsky.feed.post';

disp('Προσπάθεια σύνδεσης στο WebSocket...');


try
    time=14400.0;
    % 2. Δημιουργία σύνδεσης χρησιμοποιώντας τη βιβλιοθήκη της Python
    ws = py.websocket.create_connection(uri);
    fprintf('Η σύνδεση επιτεύχθηκε! Συλλογή δεδομένων για %d δευτερόλεπτα...', time);
    
    messages_received = 0;
    first_message_data = struct();
    
    % 3. Έναρξη χρονομέτρου
    tic; 
    max_length = 0;

    % Διαβάζουμε μηνύματα για 5 δευτερόλεπτα
    while toc < time
        % Λήψη δεδομένων (ως Python string)
        result = ws.recv();
        messages_received = messages_received + 1;
        
        current_length = length(char(result));
        if current_length > max_length
            max_length = current_length;
        end

        % Αν είναι το πρώτο μήνυμα, το αναλύουμε
        if messages_received == 1
            % Μετατροπή του Python string σε MATLAB char array
            json_str = char(result);
            
            % Αποκωδικοποίηση του JSON σε MATLAB struct
            first_message_data = jsondecode(json_str);
        end
    end

    fprintf('Μέγιστο μέγεθος μηνύματος: %d χαρακτήρες\n', max_length);

    % 4. Υπολογισμός ρυθμού (Hz)
    elapsed_time = toc;
    hz = messages_received / elapsed_time;
    
    disp('--------------------------------------------------');
    fprintf('ΣΤΑΤΙΣΤΙΚΑ ΔΙΚΤΥΟΥ:\n');
    fprintf('Λήφθηκαν %d μηνύματα σε %.2f δευτερόλεπτα.\n', messages_received, elapsed_time);
    fprintf('Εκτιμώμενος ρυθμός ροής (Rate): %.2f Hz\n', hz);
    disp('--------------------------------------------------');
    
    % 5. Εμφάνιση της δομής του JSON
    disp('ΔΟΜΗ ΠΡΩΤΟΥ ΜΗΝΥΜΑΤΟΣ JSON:');
    disp(first_message_data);
    
    % Έλεγχος για τα συγκεκριμένα πεδία που ανέφερες
    disp('--- Ανάλυση υπο-πεδίων ---');
    if isfield(first_message_data, 'commit')
        disp('Βρέθηκε το πεδίο "commit":');
        disp(first_message_data.commit);
    end
    
    if isfield(first_message_data, 'identity')
        disp('Βρέθηκε το πεδίο "identity":');
    end
    
    if isfield(first_message_data, 'account')
        disp('Βρέθηκε το πεδίο "account":');
    end
    
    % 6. Κλείσιμο σύνδεσης
    ws.close();
    disp('Η σύνδεση έκλεισε με επιτυχία.');

catch ME
    disp('Προέκυψε σφάλμα:');
    disp(ME.message);
    % Προσπάθεια κλεισίματος σε περίπτωση σφάλματος
    if exist('ws', 'var')
        ws.close();
    end
end