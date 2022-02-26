# Synchronization Strategies for Linked Lists 

### Contributors
#### - Poutas Sokratis (poutasok@gmail.com)
#### - Tzomaka Afroditi (afrodititzomaka@gmail.com)

---

### Four synchronization schemes that can be used in multithreaded applications that handle linked lists

- Fine-Grained
- Optimistic 
- Lazy
- Non-Blocking

### All implementations are C versions of Java code and ideas presented in  *"Maurice Herlihy, Nir Shavit - The Art of Multiprocessor Programming"*

Note: The locks used are of type **pthread_spinlock_t**.