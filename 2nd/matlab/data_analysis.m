clc;
clear;
close all;

set(groot, 'defaultFigureRenderer', 'painters');

% 1. Φόρτωση των δεδομένων
% Η readmatrix διαβάζει το CSV. Αν η 1η γραμμή έχει γράμματα, την αγνοεί αυτόματα.
filename = 'metrics_log.txt';
try
    data = readmatrix(filename);
catch
    error('Δεν βρέθηκε το αρχείο metrics_log.txt. Βεβαιώσου ότι είναι στον ίδιο φάκελο.');
end

% 2. Ανάθεση των 8 στηλών σε μεταβλητές
seconds      = data(:, 1);
nanoseconds  = data(:, 2);
commit_cnt   = data(:, 3);
identity_cnt = data(:, 4);
account_cnt  = data(:, 5);
info_cnt     = data(:, 6);
buffer_pct   = data(:, 7);
cpu_pct      = data(:, 8);

% 3. Μαθηματικοί Υπολογισμοί
% Α. Συνολικός Χρόνος και Σχετικός Χρόνος (για τον άξονα Χ)
time_absolute = seconds + (nanoseconds * 1e-9); % Συνένωση sec και nsec
time_relative = time_absolute - time_absolute(1); % Ξεκινάμε το γράφημα από το 0

% Β. Υπολογισμός Jitter (σε milliseconds)
% Βρίσκουμε τη διαφορά χρόνου μεταξύ συνεχόμενων καταγραφών.
% Ο ιδανικός χρόνος είναι 1.0 δευτερόλεπτα.
delta_t = diff(time_absolute); 
jitter_sec = delta_t - 1.0; % Απόκλιση από το 1 δευτερόλεπτο
jitter_ms = [0; jitter_sec * 1000]; % Προσθέτουμε ένα 0 στην αρχή για να ταιριάζει το μέγεθος των πινάκων

% Γ. Υπολογισμός Ρυθμού Μηνυμάτων (Hz)
% Αφού η καταγραφή γίνεται ανά 1 δευτερόλεπτο, το άθροισμα = Μηνύματα/sec (Hz)
total_hz = commit_cnt + identity_cnt + account_cnt + info_cnt;

% =========================================================
% 4. Σχεδιασμός Γραφημάτων (Plots)
% =========================================================

figure('Name', 'Ανάλυση Συστήματος Firehose', 'NumberTitle', 'off', 'Position', [100, 100, 1000, 800]);

% --- Διάγραμμα 1: Jitter ---
subplot(3, 1, 1);
plot(time_relative, jitter_ms, 'b-', 'LineWidth', 1.2);
hold on;
yline(0, 'r--', 'Ιδανικό (0 ms)', 'LabelHorizontalAlignment', 'left', 'LineWidth', 1.5);
title('Διάγραμμα Jitter Περιοδικού Νήματος (Ακρίβεια Timer)');
xlabel('Χρόνος (s)');
ylabel('Jitter (ms)');
grid on;

% --- Διάγραμμα 2: Φόρτος & Buffer (Διπλός Άξονας) ---
subplot(3, 1, 2);

yyaxis left; % Αριστερός άξονας (Μηνύματα)
plot(time_relative, total_hz, 'Color', [0 0.4470 0.7410], 'LineWidth', 1.5);
ylabel('Συνολικός Ρυθμός Μηνυμάτων (Hz)');

yyaxis right; % Δεξιός άξονας (Buffer)
plot(time_relative, buffer_pct, 'Color', [0.8500 0.3250 0.0980], 'LineWidth', 1.5);
ylabel('Πληρότητα Buffer (%)');
ylim([0 100]); % Το buffer είναι πάντα από 0% έως 100%

title('Συσχέτιση Δικτυακού Φόρτου (Burstiness) και Πληρότητας Κυκλικού Buffer');
xlabel('Χρόνος (s)');
grid on;

% --- Διάγραμμα 3: CPU (Scatter Plot) ---
subplot(3, 1, 3);
plot(total_hz, cpu_pct, '.', 'Color', [0.4660 0.6740 0.1880], 'MarkerSize', 12);
title('Απασχόληση CPU σε σχέση με τον ρυθμό μηνυμάτων');
xlabel('Ρυθμός Εισερχόμενων Μηνυμάτων (Hz)');
ylabel('Χρήση CPU (%)');
ylim([0 100]); % Η CPU είναι πάντα από 0% έως 100%
grid on;

sgtitle('Ανάλυση Μετρικών Απόδοσης - Bluesky Jetstream Firehose');
