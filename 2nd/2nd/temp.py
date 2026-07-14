# Ονομάστε το αρχείο σας "input_data.csv" και βάλτε το στον ίδιο φάκελο με αυτό το script.
# Το αποτέλεσμα θα αποθηκευτεί στο αρχείο "formatted_monitor_log.txt"

input_file = "metrics_log_10_7.txt"
output_file = "formatted_monitor_log.txt"

with open(input_file, 'r', encoding='utf-8') as f_in, open(output_file, 'w', encoding='utf-8') as f_out:
    # Επεξεργασία γραμμή προς γραμμή
    for idx, line in enumerate(f_in, start=1):
        if not line.strip():
            continue
            
        parts = line.strip().split(',')
        # Σιγουρευόμαστε ότι η γραμμή έχει αρκετές στήλες
        if len(parts) >= 8:
            time= parts[0]
            commits = parts[2]
            identities = parts[3]
            accounts = parts[4]
            infos = parts[5]
            oura = parts[6]
            cpu = parts[7]
            
            # Δημιουργία της επιθυμητής μορφής
            formatted_line = (f"[Monitor] --- Tick {time} --- \n Δεδομένα : "
                              f"Commits: {commits} | Identities: {identities} | "
                              f"Accounts: {accounts} | Infos: {infos} \n Σύστημα  : "
                              f"Ουρά:  {oura}% | CPU:  {cpu}%\n")
            f_out.write(formatted_line)

print("Η μετατροπή ολοκληρώθηκε με επιτυχία!")