# ECE531 Final Project - Thermostat Server
# Server code for EC2 instance to manage thermostat device.
# Artip Nakchinda
# 
# Acknowledgement:
#   - Google Gemini was used to assist with guidance and suggestions for implementation.

# Server Requirements:
# - MongoDB running on instance.
# - To run through terminal on EC2: python3 thermo_serv.py

from flask import Flask, request, jsonify
from pymongo import MongoClient
from bson.objectid import ObjectId
from bson.errors import InvalidId
import logging
import re # For validating time format

app = Flask(__name__)

# Database Connection
client = MongoClient('mongodb://localhost:27017/')
db = client.thermostat_db
thermostats = db.thermostats # store thermostat data and programs

logging.basicConfig(level=logging.INFO)

# Program Validation
def is_valid_program(program_data):
    """
    Validates the structure of a thermostat program.
    - Must be a list.
    - Must contain 1-3 entries.
    - Each entry must have 'time' (HH:MM) and 'temp' (double) celsius.
    """
    if not isinstance(program_data, list):
        return False, "Program must be a list." 
    if not 1 <= len(program_data) <= 3:
        return False, "Program must contain between 1 and 3 setpoints."

    # HH:MM format (24 hr time)
    time_regex = re.compile(r'^([01]\d|2[0-3]):([0-5]\d)$') 

    for entry in program_data:
        if not isinstance(entry, dict):
            return False, "Each program entry must be an object."
        if 'time' not in entry or 'temp' not in entry:
            return False, "Each program entry must contain 'time' and 'temp' keys."
        if not isinstance(entry['time'], str) or not time_regex.match(entry['time']):
            return False, f"Invalid time format for '{entry['time']}'. Must be HH:MM."
        if not isinstance(entry['temp'], (int, float)):
            return False, f"Temperature must be a number for time '{entry['time']}'."
            
    return True, ""


# API Routes
@app.route('/thermostat', methods=['POST'])
def register_thermostat():
    # Create new thermostat record w/ default program.
    try:
        initial_data = request.get_json() if request.data else {}
        
        initial_data['program'] = [
            {'time': '07:00', 'temp': 21.0}
        ]
        
        result = thermostats.insert_one(initial_data)
        new_id = str(result.inserted_id)
        
        app.logger.info(f"Registered new thermostat with ID: {new_id}")
        return jsonify(id=new_id), 201
    except Exception as e:
        app.logger.error(f"Error in register_thermostat: {e}")
        return jsonify(error="Internal server error"), 500

@app.route('/thermostat/<thermostat_id>', methods=['GET', 'DELETE'])
def handle_thermostat(thermostat_id):
    # GET: Fetches the entire record for a thermostat.
    # DELETE: Deletes a thermostat record.
    try:
        obj_id = ObjectId(thermostat_id)
        
        if request.method == 'GET':
            device = thermostats.find_one({'_id': obj_id})
            if device:
                device['_id'] = str(device['_id'])
                return jsonify(device)
            else:
                return jsonify(error="Thermostat not found"), 404

        elif request.method == 'DELETE':
            result = thermostats.delete_one({'_id': obj_id})
            if result.deleted_count == 1:
                app.logger.info(f"Deleted thermostat with ID: {thermostat_id}")
                return jsonify(message="Thermostat deleted successfully")
            else:
                return jsonify(error="Thermostat not found"), 404

    except InvalidId:
        return jsonify(error="Invalid thermostat ID format"), 400
    except Exception as e:
        app.logger.error(f"Error in handle_thermostat: {e}")
        return jsonify(error="Internal server error"), 500

@app.route('/thermostat/<thermostat_id>/program', methods=['GET', 'PUT'])
def handle_program(thermostat_id):
   # GET:  Fetch current program schedule for a thermostat.
   # PUT:  Update program schedule with new settings.
    try:
        obj_id = ObjectId(thermostat_id)
        device = thermostats.find_one({'_id': obj_id})

        if not device:
            return jsonify(error="Cannot find thermostat"), 404

        if request.method == 'GET':
            app.logger.info(f"GET program for thermostat {thermostat_id}")
            return jsonify(device.get('program', []))

        elif request.method == 'PUT':
            new_program_data = request.get_json()
            if not new_program_data or 'program' not in new_program_data:
                return jsonify(error="Request must contain a 'program' key."), 400
            
            is_valid, error_message = is_valid_program(new_program_data['program'])
            if not is_valid:
                return jsonify(error=error_message), 400
            
            thermostats.update_one(
                {'_id': obj_id},
                {'$set': {'program': new_program_data['program']}}
            )
            app.logger.info(f"Updated program for thermostat {thermostat_id}")
            return jsonify(success=True, program=new_program_data['program'])

    except InvalidId:
        return jsonify(error="Invalid thermostat ID format"), 400
    except Exception as e:
        app.logger.error(f"Error in handle_program: {e}")
        return jsonify(error="Internal server error"), 500


@app.route('/thermostat/<thermostat_id>/status', methods=['POST'])
def update_status(thermostat_id):
    # Receive status update from thermostat (current temp and heater state).
    try:
        obj_id = ObjectId(thermostat_id)
        status_data = request.get_json()

        if not status_data or 'current_temp' not in status_data or 'heater_on' not in status_data:
            return jsonify(error="Invalid status data."), 400
        
        thermostats.update_one(
            {'_id': obj_id},
            {'$set': {
                'last_status': status_data,
                'last_updated': '$currentDate'
            }}
        )
        app.logger.info(f"Received status update from thermostat {thermostat_id}")
        return jsonify(success=True)

    except InvalidId:
        return jsonify(error="Invalid thermostat ID format"), 400
    except Exception as e:
        app.logger.error(f"Error in update_status: {e}")
        return jsonify(error="Internal server error"), 500


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5031, debug=False)
