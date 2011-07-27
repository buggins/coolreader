package org.coolreader.crengine;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.HashMap;
import java.util.Locale;

import android.content.Context;

/**
 * Wrapper for android.speech.tts.TextToSpeech
 * 
 * For compatibility with Android 1.5
 */
public class TTS {
	
	public static final Logger log = L.create("tts");
	
	// constants from TextToSpeech
	public final static String	ACTION_TTS_QUEUE_PROCESSING_COMPLETED="android.speech.tts.TTS_QUEUE_PROCESSING_COMPLETED";	//Broadcast Action: The TextToSpeech synthesizer has completed processing of all the text in the speech queue.
	public final static int	ERROR=1;	//Denotes a generic operation failure.
	public final static int	LANG_AVAILABLE=0;	//Denotes the language is available for the language by the locale, but not the country and variant.
	public final static int	LANG_COUNTRY_AVAILABLE=1;	//Denotes the language is available for the language and country specified by the locale, but not the variant.
	public final static int	LANG_COUNTRY_VAR_AVAILABLE=2;	//Denotes the language is available exactly as specified by the locale.
	public final static int	LANG_MISSING_DATA=-1;	//Denotes the language data is missing.
	public final static int	LANG_NOT_SUPPORTED=-2;	//Denotes the language is not supported.
	public final static int	QUEUE_ADD=1;	//Queue mode where the new entry is added at the end of the playback queue.
	public final static int	QUEUE_FLUSH=0;	//Queue mode where all entries in the playback queue (media to be played and text to be synthesized) are dropped and replaced by the new entry.
	public final static int	SUCCESS=0;  //Denotes a successful operation.
	public final static String KEY_PARAM_UTTERANCE_ID = "utteranceId";
	
	private static Class<?> textToSpeechClass;
	private static Constructor<?> textToSpeech_constructor;
	private static Class<?> onInitListenerClass;
	//private static Method onInitListener_onInit; //	void onInit(int status)
	private static Class<?> onUtteranceCompletedListenerClass;
	//private static Method onUtteranceCompletedListener_onUtteranceCompleted;
	
	private static Method textToSpeech_addEarcon; //int addEarcon(String earcon, String filename); // Adds a mapping between a string of text and a sound file.
	private static Method textToSpeech_addEarcon2; //int 	addEarcon(String earcon, String packagename, int resourceId); // Adds a mapping between a string of text and a sound resource in a package.
	private static Method textToSpeech_addSpeech; //int 	addSpeech(String text, String packagename, int resourceId); // Adds a mapping between a string of text and a sound resource in a package.
	private static Method textToSpeech_addSpeech2; //int 	addSpeech(String text, String filename); //Adds a mapping between a string of text and a sound file.
	private static Method textToSpeech_areDefaultsEnforced; //boolean 	areDefaultsEnforced(); // Returns whether or not the user is forcing their defaults to override the Text-To-Speech settings set by applications.
	private static Method textToSpeech_getDefaultEngine; //String 	getDefaultEngine(); // Gets the packagename of the default speech synthesis engine.
	private static Method textToSpeech_getLanguage; //Locale 	getLanguage(); // Returns a Locale instance describing the language currently being used by the TextToSpeech engine.
	private static Method textToSpeech_isLanguageAvailable; //int 	isLanguageAvailable(Locale loc); // Checks if the specified language as represented by the Locale is available and supported.
	private static Method textToSpeech_isSpeaking; //boolean isSpeaking(); // Returns whether or not the TextToSpeech engine is busy speaking.
	private static Method textToSpeech_playEarcon; //int 	playEarcon(String earcon, int queueMode, HashMap<String, String> params); // Plays the earcon using the specified queueing mode and parameters.
	private static Method textToSpeech_playSilence; //int 	playSilence(long durationInMs, int queueMode, HashMap<String, String> params); // Plays silence for the specified amount of time using the specified queue mode.
	private static Method textToSpeech_setEngineByPackageName; //int 	setEngineByPackageName(String enginePackageName); // Sets the speech synthesis engine to be used by its packagename.
	private static Method textToSpeech_setLanguage; //int 	setLanguage(Locale loc); // Sets the language for the TextToSpeech engine.
	private static Method textToSpeech_setOnUtteranceCompletedListener; //int 	setOnUtteranceCompletedListener(TextToSpeech.OnUtteranceCompletedListener listener); // Sets the OnUtteranceCompletedListener that will fire when an utterance completes.
	private static Method textToSpeech_setPitch; //int 	setPitch(float pitch); // Sets the speech pitch for the TextToSpeech engine.
	private static Method textToSpeech_setSpeechRate; //int 	setSpeechRate(float speechRate); // Sets the speech rate for the TextToSpeech engine.
	private static Method textToSpeech_shutdown; //void 	shutdown() ; // Releases the resources used by the TextToSpeech engine.
	private static Method textToSpeech_speak; //int 	speak(String text, int queueMode, HashMap<String, String> params); // Speaks the string using the specified queuing strategy and speech parameters.
	private static Method textToSpeech_stop; //int 	stop() ; // Interrupts the current utterance (whether played or rendered to file) and discards other utterances in the queue.
	private static Method textToSpeech_synthesizeToFile; //int 	synthesizeToFile(String text, HashMap<String, String> params, String filename); // Synthesizes the given text to a file using the specified parameters.

	private Object tts;
	private boolean initialized;
	
	/**
	 * @param listener is listener to call on init finished
	 * @return proxy
	 */
	private Object createOnInitProxy( final OnInitListener listener ) {
		InvocationHandler handler = new InvocationHandler() {

			@Override
			public Object invoke(Object proxy, Method method, Object[] args)
					throws Throwable {
                log.d("invoking OnInit - " + method.getName());
				if ( "onInit".equals(method.getName()) ) {
					int status = (Integer)(args[0]);
					log.i("OnInitListener.onInit() is called: status=" + status);
					if ( status==SUCCESS )
						initialized = true;
					listener.onInit(status);
				}
				return null;
			}
			
		};
		return Proxy.newProxyInstance(
				onInitListenerClass.getClassLoader(),
		        new Class[] { onInitListenerClass },
		        handler);		
	}
	
	/**
	 * @param listener is listener to call on init finished
	 * @return proxy
	 */
	private Object createOnUtteranceCompletedListener( final OnUtteranceCompletedListener listener ) {
		InvocationHandler handler = new InvocationHandler() {

			@Override
			public Object invoke(Object proxy, Method method, Object[] args)
					throws Throwable {
				log.d("invoking OnUtteranceCompletedListener - " + method.getName());
                if ( "onUtteranceCompleted".equals(method.getName()) ) {
					String id = (String)(args[0]);
					log.d("OnUtteranceCompletedListener.onUtteranceCompleted() is called: id=" + id);
					listener.onUtteranceCompleted(id);
				}
				return null;
			}
			
		};
		return Proxy.newProxyInstance(
				onUtteranceCompletedListenerClass.getClassLoader(),
		        new Class[] { onUtteranceCompletedListenerClass },
		        handler);		
	}
	
	private static boolean classesFound;
	static {
		try {
			onInitListenerClass = Class.forName("android.speech.tts.TextToSpeech$OnInitListener");
			//onInitListener_onInit = onInitListenerClass.getMethod("onInit", new Class[] {int.class});
			onUtteranceCompletedListenerClass = Class.forName("android.speech.tts.TextToSpeech$OnUtteranceCompletedListener");
			//onUtteranceCompletedListener_onUtteranceCompleted = onUtteranceCompletedListenerClass.getMethod("onUtteranceCompleted", new Class[] {String.class});
			textToSpeechClass = Class.forName("android.speech.tts.TextToSpeech");
			textToSpeech_constructor = textToSpeechClass.getConstructor(new Class[] {Context.class, onInitListenerClass}); 
			textToSpeech_addEarcon = textToSpeechClass.getMethod("addEarcon", new Class[] {String.class, String.class}); //int addEarcon(String earcon, String filename); // Adds a mapping between a string of text and a sound file.
			textToSpeech_addEarcon2 = textToSpeechClass.getMethod("addEarcon", new Class[] {String.class, String.class, int.class}); //int 	addEarcon(String earcon, String packagename, int resourceId); // Adds a mapping between a string of text and a sound resource in a package.
			textToSpeech_addSpeech = textToSpeechClass.getMethod("addSpeech", new Class[] {String.class, String.class, int.class}); //int 	addSpeech(String text, String packagename, int resourceId); // Adds a mapping between a string of text and a sound resource in a package.
			textToSpeech_addSpeech2 = textToSpeechClass.getMethod("addSpeech", new Class[] {String.class, String.class}); //int 	addSpeech(String text, String filename); //Adds a mapping between a string of text and a sound file.
			textToSpeech_areDefaultsEnforced = textToSpeechClass.getMethod("areDefaultsEnforced", new Class[] {}); //boolean 	areDefaultsEnforced(); // Returns whether or not the user is forcing their defaults to override the Text-To-Speech settings set by applications.
			textToSpeech_getDefaultEngine = textToSpeechClass.getMethod("getDefaultEngine", new Class[] {}); //String 	getDefaultEngine(); // Gets the packagename of the default speech synthesis engine.
			textToSpeech_getLanguage = textToSpeechClass.getMethod("getLanguage", new Class[] {}); //Locale 	getLanguage(); // Returns a Locale instance describing the language currently being used by the TextToSpeech engine.
			textToSpeech_isLanguageAvailable = textToSpeechClass.getMethod("isLanguageAvailable", new Class[] {Locale.class}); //int 	isLanguageAvailable(Locale loc); // Checks if the specified language as represented by the Locale is available and supported.
			textToSpeech_isSpeaking = textToSpeechClass.getMethod("isSpeaking", new Class[] {}); //boolean isSpeaking(); // Returns whether or not the TextToSpeech engine is busy speaking.
			textToSpeech_playEarcon = textToSpeechClass.getMethod("playEarcon", new Class[] {String.class, int.class, HashMap.class}); //int 	playEarcon(String earcon, int queueMode, HashMap<String, String> params); // Plays the earcon using the specified queueing mode and parameters.
			textToSpeech_playSilence = textToSpeechClass.getMethod("playSilence", new Class[] {long.class, int.class, HashMap.class}); //int 	playSilence(long durationInMs, int queueMode, HashMap<String, String> params); // Plays silence for the specified amount of time using the specified queue mode.
			textToSpeech_setEngineByPackageName = textToSpeechClass.getMethod("setEngineByPackageName", new Class[] {String.class}); //int 	setEngineByPackageName(String enginePackageName); // Sets the speech synthesis engine to be used by its packagename.
			textToSpeech_setLanguage = textToSpeechClass.getMethod("setLanguage", new Class[] {Locale.class}); //int 	setLanguage(Locale loc); // Sets the language for the TextToSpeech engine.
			textToSpeech_setOnUtteranceCompletedListener = textToSpeechClass.getMethod("setOnUtteranceCompletedListener", new Class[] {onUtteranceCompletedListenerClass}); //int 	setOnUtteranceCompletedListener(TextToSpeech.OnUtteranceCompletedListener listener); // Sets the OnUtteranceCompletedListener that will fire when an utterance completes.
			textToSpeech_setPitch = textToSpeechClass.getMethod("setPitch", new Class[] {float.class}); //int 	setPitch(float pitch); // Sets the speech pitch for the TextToSpeech engine.
			textToSpeech_setSpeechRate = textToSpeechClass.getMethod("setSpeechRate", new Class[] {float.class}); //int 	setSpeechRate(float speechRate); // Sets the speech rate for the TextToSpeech engine.
			textToSpeech_shutdown = textToSpeechClass.getMethod("shutdown", new Class[] {}); //void 	shutdown() ; // Releases the resources used by the TextToSpeech engine.
			textToSpeech_speak = textToSpeechClass.getMethod("speak", new Class[] {String.class, int.class, HashMap.class}); //int 	speak(String text, int queueMode, HashMap<String, String> params); // Speaks the string using the specified queuing strategy and speech parameters.
			textToSpeech_stop = textToSpeechClass.getMethod("stop", new Class[] {}); //int 	stop() ; // Interrupts the current utterance (whether played or rendered to file) and discards other utterances in the queue.
			textToSpeech_synthesizeToFile = textToSpeechClass.getMethod("synthesizeToFile", new Class[] {String.class, HashMap.class, String.class}); //int 	synthesizeToFile(String text, HashMap<String, String> params, String filename); // Synthesizes the given text to a file using the specified parameters.
			classesFound = true;
			L.i("TTS classes initialized successfully");
		} catch ( Exception e ) {
			L.e("Exception while initializing TTS classes: tts will be disabled", e);
			classesFound = false;
		}
	}
	
	public interface OnInitListener {
		void onInit(int status);
	}
	
	public interface OnUtteranceCompletedListener {
		/**
		 * Called to signal the completion of the synthesis of the utterance that was identified with the string parameter.
		 * @param utteranceId
		 */
		void 	onUtteranceCompleted(String utteranceId);
	}
	
	public interface OnTTSCreatedListener {
		void onCreated(TTS tts);
	}
	

	public TTS(Context context, OnInitListener listener ) {
		if ( !classesFound ) {
			L.e("Cannot create TTS object : TTS classes not initialized");
			throw new IllegalStateException("Cannot instanciate TextToSpeech");
		}
		try {
			tts = textToSpeech_constructor.newInstance(context, createOnInitProxy(listener));
			L.i("TTS object created successfully");
//	    	setOnUtteranceCompletedListener(new TTS.OnUtteranceCompletedListener() {
//				
//				@Override
//				public void onUtteranceCompleted(String utteranceId) {
//					L.i("TTS utterance completed: " + utteranceId);
//					// TODO
//				}
//			});
		} catch ( InvocationTargetException e ) {
			classesFound = false;
			L.e("Cannot create TTS object", e);
			throw new IllegalStateException("Cannot instanciate TextToSpeech");
		} catch (IllegalArgumentException e) {
			classesFound = false;
			L.e("Cannot create TTS object", e);
			throw new IllegalStateException("Cannot instanciate TextToSpeech");
		} catch (InstantiationException e) {
			classesFound = false;
			L.e("Cannot create TTS object", e);
			throw new IllegalStateException("Cannot instanciate TextToSpeech");
		} catch (IllegalAccessException e) {
			classesFound = false;
			L.e("Cannot create TTS object", e);
			throw new IllegalStateException("Cannot instanciate TextToSpeech");
		}
	}

	// Adds a mapping between a string of text and a sound file.
	public int addEarcon(String earcon, String filename) {
		try {
			return (Integer)textToSpeech_addEarcon.invoke(tts, earcon, filename);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	
	// Adds a mapping between a string of text and a sound resource in a package.
	public int 	addEarcon(String earcon, String packagename, int resourceId) {
		try {
			return (Integer)textToSpeech_addEarcon2.invoke(tts, packagename, resourceId);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	
	// Adds a mapping between a string of text and a sound resource in a package.
	public int 	addSpeech(String text, String packagename, int resourceId) {
		try {
			return (Integer)textToSpeech_addSpeech.invoke(tts, text, packagename, resourceId);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	//Adds a mapping between a string of text and a sound file.
	public int 	addSpeech(String text, String filename) {
		try {
			return (Integer)textToSpeech_addSpeech2.invoke(tts, text, filename);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Returns whether or not the user is forcing their defaults to override the Text-To-Speech settings set by applications.
	public boolean areDefaultsEnforced() {
		try {
			return (Boolean)textToSpeech_areDefaultsEnforced.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Gets the packagename of the default speech synthesis engine.
	public String 	getDefaultEngine() {
		try {
			return (String)textToSpeech_getDefaultEngine.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Returns a Locale instance describing the language currently being used by the TextToSpeech engine.
	public Locale 	getLanguage() {
		try {
			return (Locale)textToSpeech_getLanguage.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Checks if the specified language as represented by the Locale is available and supported.
	public int 	isLanguageAvailable(Locale loc) {
		try {
			return (Integer)textToSpeech_isLanguageAvailable.invoke(tts, loc);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Returns whether or not the TextToSpeech engine is busy speaking.
	public boolean isSpeaking() {
		try {
			return (Boolean)textToSpeech_isSpeaking.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Plays the earcon using the specified queueing mode and parameters.
	public int playEarcon(String earcon, int queueMode, HashMap<String, String> params) {
		try {
			return (Integer)textToSpeech_playEarcon.invoke(tts, earcon, queueMode, params);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Plays silence for the specified amount of time using the specified queue mode.
	public int 	playSilence(long durationInMs, int queueMode, HashMap<String, String> params) {
		try {
			return (Integer)textToSpeech_playSilence.invoke(tts, durationInMs, queueMode, params);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Sets the speech synthesis engine to be used by its packagename.
	public int setEngineByPackageName(String enginePackageName) {
		try {
			return (Integer)textToSpeech_setEngineByPackageName.invoke(tts, enginePackageName);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Sets the language for the TextToSpeech engine.
	public int setLanguage(Locale loc) {
		try {
			return (Integer)textToSpeech_setLanguage.invoke(tts, loc);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Sets the OnUtteranceCompletedListener that will fire when an utterance completes.
	public int setOnUtteranceCompletedListener(OnUtteranceCompletedListener listener) {
		try {
			return (Integer)textToSpeech_setOnUtteranceCompletedListener.invoke(tts, createOnUtteranceCompletedListener(listener));
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Sets the speech pitch for the TextToSpeech engine.
	public int 	setPitch(float pitch) {
		try {
			return (Integer)textToSpeech_setPitch.invoke(tts, pitch);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Sets the speech rate for the TextToSpeech engine.
	public int 	setSpeechRate(float speechRate) {
		try {
			return (Integer)textToSpeech_setSpeechRate.invoke(tts, speechRate);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}

	// Releases the resources used by the TextToSpeech engine.
	public void shutdown() {
		if ( tts!=null && initialized )
		try {
			initialized = false;
			textToSpeech_shutdown.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}

	// Speaks the string using the specified queuing strategy and speech parameters.
	public int speak(String text, int queueMode, HashMap<String, String> params) {
        try {
            L.v("speak(" + text + ")");
            int res = (Integer) textToSpeech_speak.invoke(tts, text, queueMode,
                    params);
            L.v("speak() returned " + res);
            return res;
        } catch (Exception e) {
            L.e("Exception while calling tts", e);
            throw new IllegalStateException(e);
        }
	}
	// Interrupts the current utterance (whether played or rendered to file) and discards other utterances in the queue.
	public int 	stop() {
		try {
			return (Integer)textToSpeech_stop.invoke(tts);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	// Synthesizes the given text to a file using the specified parameters.
	public int 	synthesizeToFile(String text, HashMap<String, String> params, String filename) {
		try {
			return (Integer)textToSpeech_synthesizeToFile.invoke(tts, text, params, filename);
		} catch ( Exception e ) {
			L.e("Exception while calling tts", e);
			throw new IllegalStateException(e);
		}
	}
	
	public boolean isInitialized() {
		return false;
	}
	
	public static boolean isFound() {
		return classesFound;
	}
}
