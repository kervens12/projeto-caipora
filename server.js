import express from "express";
import cors from "cors";
import { SerialPort, ReadlineParser } from "serialport";
import axios from "axios";

// =====================================================
// EXPRESS
// =====================================================

const app = express();

app.use(cors());

app.use(express.json());

// =====================================================
// CONFIGURAÇÕES
// =====================================================

// MUDE PARA SUA PORTA COM

const ARDUINO_PORT = "COM13";

const BAUD_RATE = 115200;

const SERVER_PORT = 3001;

// SUA API

const API_URL =
  "https://api-cvehu2aruq-uc.a.run.app/alertas";

// =====================================================
// SERIAL
// =====================================================

const port = new SerialPort({

  path: ARDUINO_PORT,

  baudRate: BAUD_RATE,
});

const parser = port.pipe(

  new ReadlineParser({

    delimiter: "\n",
  })
);

// =====================================================
// ÚLTIMO ALERTA
// =====================================================

let ultimoAlerta = null;

// =====================================================
// SERIAL OPEN
// =====================================================

port.on("open", () => {

  console.log("\n================================");

  console.log(" CAIPORA GATEWAY ");

  console.log("================================");

  console.log("Porta Serial:", ARDUINO_PORT);

  console.log("API:", API_URL);

  console.log("================================\n");
});

// =====================================================
// RECEBER SERIAL
// =====================================================

parser.on("data", async (line) => {

  line = line.trim();

  console.log("SERIAL:", line);

  // =============================================
  // IGNORA LINHAS NORMAIS
  // =============================================

  if (!line.startsWith("{")) {

    return;
  }

  try {

    // =========================================
    // CONVERTE JSON
    // =========================================

    const data = JSON.parse(line);

    ultimoAlerta = data;

    console.log("\n================================");

    console.log(" ALERTA RECEBIDO ");

    console.log("================================");

    console.log(data);

    console.log("================================\n");

    // =========================================
    // ENVIA PARA API
    // =========================================

    const response = await axios.post(

      API_URL,

      data
    );

    console.log("\n================================");

    console.log(" ALERTA ENVIADO ");

    console.log("================================");

    console.log("Status:", response.status);

    console.log(response.data);

    console.log("================================\n");

  } catch (error) {

    console.log("\n================================");

    console.log(" ERRO ");

    console.log("================================");

    if (error.response) {

      console.log(error.response.data);

    } else {

      console.log(error.message);
    }

    console.log("================================\n");
  }
});

// =====================================================
// STATUS SERVIDOR
// =====================================================

app.get("/", (req, res) => {

  res.json({

    status: "online",

    serial: ARDUINO_PORT,

    api: API_URL,
  });
});

// =====================================================
// ÚLTIMO ALERTA
// =====================================================

app.get("/alerta", (req, res) => {

  if (!ultimoAlerta) {

    return res.json({

      status: "sem_alerta",
    });
  }

  res.json({

    status: "alerta_recebido",

    alerta: ultimoAlerta,

    timestamp: new Date().toISOString(),
  });
});

// =====================================================
// START SERVER
// =====================================================

app.listen(SERVER_PORT, () => {

  console.log(

    `Servidor rodando: http://localhost:${SERVER_PORT}`
  );

  console.log(

    `Último alerta: http://localhost:${SERVER_PORT}/alerta`
  );
});