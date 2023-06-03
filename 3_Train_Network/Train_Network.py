# In this script a network is trained with the data from the ESP32

def main():

    import numpy as np
    import pandas as pd
    import matplotlib.pyplot as plt
    import tensorflow as tf
    from keras.models import Sequential
    from keras.layers import Dense, Input
    from keras.optimizers import Adam

    Data = pd.read_csv("2_Collect_Serial_Data/Serial_Data_Unwuchtmotor_training.csv",header=None)
    Data = Data.values

    print(Data.shape)

    plt.figure()
    plt.plot(Data[0,:], label="Eingabevektor")
    plt.xlabel("Beschleunigungen")
    plt.ylabel("Messwerte")
    plt.title("Eingabevektor einer Schwingung")
    plt.legend(loc="upper right")
    plt.show()

    Datax = np.zeros(shape=(1,1))
    Datay = np.zeros(shape=(1,1))
    Dataz = np.zeros(shape=(1,1))
    Dataa = np.zeros(shape=(1,1))
    Datab = np.zeros(shape=(1,1))
    Datac = np.zeros(shape=(1,1))

    for i in range(len(Data[:,0])):
        Datax = np.append(Datax, Data[i,:128])
        Datay = np.append(Datay, Data[i,128:128*2])
        Dataz = np.append(Dataz, Data[i,128*2:128*3])
        Dataa = np.append(Dataa, Data[i,128*3:128*4])
        Datab = np.append(Datab, Data[i,128*4:128*5])
        Datac = np.append(Datac, Data[i,128*5:128*6])

    plt.figure()
    plt.plot(Datax, label="Translation in X")
    plt.plot(Datay, label="Translation in Y")
    plt.plot(Dataz, label="Translation in Z")
    plt.plot(Dataa, label="Rotation um A")
    plt.plot(Datab, label="Rotation um B")
    plt.plot(Datac, label="Rotation um C")
    plt.xlabel("Messwerte")
    plt.ylabel("Beschleunigung")
    plt.title("Schwingungsdaten des Unwuchtmotors")
    plt.legend(loc="center right")
    plt.show()

    Index_Train = int(np.floor(len(Data[:,0])*0.8))

    Train = Data[:Index_Train,:]
    Test = Data[Index_Train:,:]

    Train = tf.cast(Train, tf.float32)
    Test = tf.cast(Test, tf.float32)

    input_shape = (768,)

    def AutoEncoder():
        model = Sequential()
        model.add(Input(shape=input_shape, batch_size=1))
        model.add(Dense(96, activation="tanh"))
        model.add(Dense(48, activation="tanh"))
        model.add(Dense(24, activation="tanh"))
        model.add(Dense(12, activation="tanh"))
        model.add(Dense(6, activation="tanh"))
        model.add(Dense(12, activation="tanh"))
        model.add(Dense(24, activation="tanh"))
        model.add(Dense(48, activation="tanh"))
        model.add(Dense(96, activation="tanh"))
        model.add(Dense(768, activation="tanh"))
        model.summary()
        return model

    model = AutoEncoder()

    opt = Adam(
        learning_rate=0.0001,
        beta_1=0.9,
        beta_2=0.999,
        epsilon=1e-07)

    model.compile(optimizer=opt, loss="mse")

    history = model.fit(
        Train, Train,
        workers = 4,
        epochs = 2000,
        batch_size = 1,
        validation_data=(Test, Test),
        shuffle=True)

    #Plot the Training history
    plt.plot(history.history["loss"], label="Trainingsfehler")
    plt.plot(history.history["val_loss"], label="Testfehler")
    plt.title("Trainingsverlauf des Netzwerks")
    plt.xlabel("Epochen")
    plt.ylabel("Mittlerer Absoluter Fehler")
    plt.legend()
    plt.show()

    # Running Test Data through the network
    AE_Test = model.predict(Test, batch_size=1, steps=3)

    # Plot Reconstruction Train
    for i in range(3):
        plt.plot(Test[i,:], 'b')
        plt.plot(AE_Test[i,:], 'r')
        plt.fill_between(np.arange(768), AE_Test[i,:], Test[i,:], color='lightcoral')
        plt.legend(labels=["Orginaldaten", "Rekonstruktion", "Fehler"])
        plt.title("Rekonstruktion normale Testdaten")
        plt.show()

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    # tflite_model = converter.convert()

    # Convert the model to the TensorFlow Lite format with quantization
    def representative_dataset():
        for i in range(500):
            yield([Train[i,:]])

    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8  # or tf.uint8
    converter.inference_output_type = tf.int8  # or tf.uint8
    tflite_model = converter.convert()

    with open("4_Transform_Network/ESP32_Prediction_Model_Fan_3.tflite", "wb") as File:
        File.write(tflite_model)

if __name__ == "__main__":
    main()